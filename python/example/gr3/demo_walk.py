# ============================================================
# In this demo, a joystick is required to control the walking direction and velocity
# ============================================================

import time
from fourier_aurora_client import AuroraClient
import os
import numpy
import torch
import threading
import sys
import pygame
from ischedule import run_loop, schedule
from collections import deque



# Initialize a client
client = AuroraClient.get_instance(domain_id=123, robot_name="gr3")
time.sleep(1)

policy_file_path = None
policy_model = None
policy_action = None
obs_buf_stack = None
stop_event = threading.Event()

# ============================================================
# 1. Definition
# ============================================================

ROBOT_NUM_OF_JOINTS = 31
POLICY_CONTROL_NUM_OF_JOINTS = 12
POLICY_CONTROL_INDEX_OF_JOINTS = numpy.array([
    0, 1, 2, 3, 4, 5,  # left leg
    6, 7, 8, 9, 10, 11,  # right leg
])
GROUP_NAMES = [
    'left_leg', 'right_leg', 'waist', 'head', 'left_manipulator', 'right_manipulator'
]

# gr3v224 WBCYawTask config
ACTION_SCALE = 0.5
NUM_HIST = 40
CTRL_DT = 0.02
HA_INIT = numpy.array([0.88, 0.0, 0.0], dtype=float)
HA_LOWER = numpy.array([0.40, -0.3, -0.4], dtype=float)
HA_UPPER = numpy.array([0.92, 0.5, 0.4], dtype=float)
VEL_CMD_MAX = numpy.array([1.0, 0.3, 0.6], dtype=float)
VEL_CMD_FILTER = numpy.array([0.8, 0.9, 0.5], dtype=float)
JOINT_TORQUE_LIMIT = numpy.array([
    # left leg
    300.0, 140.0, 140.0, 300.0, 30.0, 10.0,
    # right leg
    300.0, 140.0, 140.0, 300.0, 30.0, 10.0,
], dtype=float)

DEFAULT_JOINT_POSITION = numpy.array([
    -0.2618, 0.0, 0.0, 0.5236, -0.2618, 0.0, # left leg
    -0.2618, 0.0, 0.0, 0.5236, -0.2618, 0.0, # right leg
    0.0, 0.0, 0.03, # waist
    0.0, 0.0, # head
    0.1, 0.15, -0.25, -0.5, 0.0, 0.0, 0.0, # left arm
    0.1, -0.15, 0.25, -0.5, 0.0, 0.0, 0.0, # right arm
], dtype=float)

KP_JOINT_SPACE = numpy.array([
    200, 200, 120, 200, 115.4, 15, # left leg
    200, 200, 120, 200, 115.4, 15, # right leg
    300, 450, 300, # waist
    100, 100, # head
    200, 200, 50, 200, 30, 30, 30, # left arm
    200, 200, 50, 200, 30, 30, 30, # right arm
], dtype=float)

KD_JOINT_SPACE = numpy.array([
    20, 20, 12, 20, 11.54, 1.5, # left leg
    20, 20, 12, 20, 11.54, 1.5, # right leg
    20, 30, 20, # waist
    8, 8, # head
    10, 10, 2, 10, 0.5, 0.5, 0.5, # left arm
    10, 10, 2, 10, 0.5, 0.5, 0.5, # right arm
], dtype=float)

GRAVITY_VECTOR = numpy.array([0.0, 0.0, -1.0], dtype=float)

term_hist = None
vel_cmd_scaled = None
height_attitude = None
is_walk_flag = 0.0
is_walk_flag_last = 0.0
rising_edge_length = 0
falling_edge_length = 0
yaw_des = 0.0
yaw_err = 0.0


def algorithm():
    global policy_model, policy_action, obs_buf_stack, commands_filtered
    global term_hist, vel_cmd_scaled, height_attitude
    global is_walk_flag, is_walk_flag_last, rising_edge_length, falling_edge_length
    global yaw_des, yaw_err


    # ============================================================
    # 2. Obtain robot states
    # ============================================================

    imu_measured_quat = client.get_base_data('quat_xyzw')
    imu_measured_angular_velocity = client.get_base_data('omega_B')
    joint_measured_position = []
    joint_measured_velocity = []
    for name in GROUP_NAMES:
        joint_measured_position.append(client.get_group_state(name, 'position'))
        joint_measured_velocity.append(client.get_group_state(name, 'velocity'))


    joint_measured_position = numpy.concatenate(joint_measured_position)
    joint_measured_velocity = numpy.concatenate(joint_measured_velocity)


    # ============================================================
    # 3. Construct velocity commands (from joystick) [lin_vel_x, lin_vel_y, ang_vel_yaw], unit: m/s, m/s, rad/s
    # ============================================================

    commands_raw = numpy.array([
        -1.0 * axis_left[1],
        -1.0 * axis_left[0],
        -1.0 * axis_right[0],
    ], dtype=float)

    vel_cmd_scaled = VEL_CMD_FILTER * vel_cmd_scaled + (1.0 - VEL_CMD_FILTER) * commands_raw
    vel_cmd_scaled[0] = numpy.clip(vel_cmd_scaled[0], -(0.2 + 2.0 * (height_attitude[0] - 0.45)), (0.2 + 2.0 * (height_attitude[0] - 0.45)))
    vel_cmd_scaled[1] = numpy.clip(vel_cmd_scaled[1], -(0.2 + 0.8 * (height_attitude[0] - 0.45)), (0.2 + 0.8 * (height_attitude[0] - 0.45)))
    vel_cmd_scaled = numpy.clip(vel_cmd_scaled, -VEL_CMD_MAX, VEL_CMD_MAX)

    is_walk_flag_last = is_walk_flag
    is_walk_flag = 1.0 if numpy.linalg.norm(vel_cmd_scaled) > 0.01 else 0.0

    if is_walk_flag > 0.5 and is_walk_flag_last < 0.5:
        rising_edge_length = NUM_HIST
    else:
        rising_edge_length -= 1

    if is_walk_flag < 0.5 and is_walk_flag_last > 0.5:
        falling_edge_length = NUM_HIST
    else:
        falling_edge_length -= 1

    if rising_edge_length >= 0:
        is_walk_flag = 1.0
    elif falling_edge_length >= 0:
        is_walk_flag = 0.0

    yaw_cur = quat_xyzw_to_yaw(imu_measured_quat)
    if abs(yaw_err) > 0.15 or is_walk_flag < 0.5:
        yaw_des = yaw_cur
    yaw_des += CTRL_DT * commands_raw[2]
    yaw_des = wrap_to_pi(yaw_des)
    yaw_err = wrap_to_pi(yaw_des - yaw_cur)

    if is_walk_flag < 0.5:
        commands_filtered[:] = 0.0
    else:
        height_attitude[2] = 0.0
        commands_filtered[:] = vel_cmd_scaled
        commands_filtered[2] += 0.2 * int(yaw_err > 0.05) - 0.2 * int(yaw_err < -0.05)
        commands_filtered[:] = numpy.clip(commands_filtered, -VEL_CMD_MAX, VEL_CMD_MAX)

    print("commands: ", commands_filtered)

    # ============================================================
    # 4. Construct observations (obs buffer)
    # ============================================================
    base_measured_quat = numpy.array([0.0, 0.0, 0.0, 1.0, ])
    base_measured_angular_velocity = numpy.array([0.0, 0.0, 0.0, ])

    for i in range(4):
        base_measured_quat[i] = imu_measured_quat[i]

    for i in range(3):
        base_measured_angular_velocity[i] = imu_measured_angular_velocity[i]

    joint_measured_position_for_policy = numpy.zeros(POLICY_CONTROL_NUM_OF_JOINTS)
    joint_measured_velocity_for_policy = numpy.zeros(POLICY_CONTROL_NUM_OF_JOINTS)

    for i in range(POLICY_CONTROL_NUM_OF_JOINTS):
        index = POLICY_CONTROL_INDEX_OF_JOINTS[i]
        joint_measured_position_for_policy[i] = joint_measured_position[index]
        joint_measured_velocity_for_policy[i] = joint_measured_velocity[index]

    if policy_action is None:
        policy_action = numpy.zeros(POLICY_CONTROL_NUM_OF_JOINTS)

    torch_base_measured_quat = torch.from_numpy(base_measured_quat).float().unsqueeze(0)
    torch_gravity_vector = torch.from_numpy(GRAVITY_VECTOR).float().unsqueeze(0)
    torch_base_project_gravity = torch_quat_rotate_inverse(torch_base_measured_quat, torch_gravity_vector)
    base_project_gravity = torch_base_project_gravity.numpy().squeeze(0)

    jpos_rel = ACTION_SCALE * (joint_measured_position_for_policy - DEFAULT_JOINT_POSITION[:POLICY_CONTROL_NUM_OF_JOINTS])
    jvel_scale = 0.05 * joint_measured_velocity_for_policy

    if term_hist is None:
        term_hist = {
            "omega_B": deque([numpy.zeros(3)] * NUM_HIST, maxlen=NUM_HIST),
            "grav_proj": deque([base_project_gravity.copy()] * NUM_HIST, maxlen=NUM_HIST),
            "is_walk": deque([numpy.array([0.0])] * NUM_HIST, maxlen=NUM_HIST),
            "vel_cmd": deque([numpy.zeros(3)] * NUM_HIST, maxlen=NUM_HIST),
            "height_attitude": deque([height_attitude.copy()] * NUM_HIST, maxlen=NUM_HIST),
            "q_rel": deque([jpos_rel.copy()] * NUM_HIST, maxlen=NUM_HIST),
            "dq": deque([numpy.zeros(POLICY_CONTROL_NUM_OF_JOINTS)] * NUM_HIST, maxlen=NUM_HIST),
            "action": deque([policy_action.copy()] * NUM_HIST, maxlen=NUM_HIST),
        }

    term_hist["omega_B"].append(numpy.asarray(base_measured_angular_velocity, dtype=float))
    term_hist["grav_proj"].append(numpy.asarray(base_project_gravity, dtype=float))
    term_hist["is_walk"].append(numpy.array([is_walk_flag], dtype=float))
    term_hist["vel_cmd"].append(numpy.asarray(commands_filtered, dtype=float))
    term_hist["height_attitude"].append(numpy.asarray(height_attitude, dtype=float))
    term_hist["q_rel"].append(jpos_rel)
    term_hist["dq"].append(jvel_scale)
    term_hist["action"].append(policy_action.copy())

    policy_input_vec = []
    for key in ["omega_B", "grav_proj", "is_walk", "vel_cmd", "height_attitude", "q_rel", "dq", "action"]:
        for value in term_hist[key]:
            policy_input_vec.extend(value.tolist())

    obs_buf_stack = torch.tensor(policy_input_vec, dtype=torch.float32)
    obs_buf_stack = torch.clamp(obs_buf_stack, -100.0, 100.0)


    # ============================================================
    # 5. Generate action
    # ============================================================

    try:
        torch_policy_action = policy_model(obs_buf_stack).detach()
    except RuntimeError:
        torch_policy_action = policy_model(obs_buf_stack.unsqueeze(0)).detach()

    if torch_policy_action.dim() > 1:
        torch_policy_action = torch_policy_action.squeeze(0)

    torch_policy_action = torch.clamp(torch_policy_action, -100.0, 100.0)
    policy_action = torch_policy_action.float().numpy()

    joint_target_position = DEFAULT_JOINT_POSITION.copy()
    joint_target_position_from_policy = policy_action * ACTION_SCALE + DEFAULT_JOINT_POSITION[:POLICY_CONTROL_NUM_OF_JOINTS]

    for i in range(POLICY_CONTROL_NUM_OF_JOINTS):
        index = POLICY_CONTROL_INDEX_OF_JOINTS[i]
        joint_target_position[index] = joint_target_position_from_policy[i]

    q_leg = joint_measured_position[POLICY_CONTROL_INDEX_OF_JOINTS]
    kp_leg = KP_JOINT_SPACE[:POLICY_CONTROL_NUM_OF_JOINTS]
    est_torque = kp_leg * (joint_target_position[:POLICY_CONTROL_NUM_OF_JOINTS] - q_leg)
    est_torque = numpy.clip(est_torque, -JOINT_TORQUE_LIMIT, JOINT_TORQUE_LIMIT)
    joint_target_position[:POLICY_CONTROL_NUM_OF_JOINTS] = est_torque / kp_leg + q_leg

    # ============================================================
    # 6. Set position command
    # ============================================================
    whole_body_pos = {"whole_body": joint_target_position.tolist()}
    client.set_joint_positions(whole_body_pos)

# ------------------ read joystick input ------------------

def joystick_listener():
    global joystick, axis_left, axis_right

    while not stop_event.is_set():
        pygame.event.get()

        axis_left = joystick.get_axis(0), joystick.get_axis(1)
        axis_right = joystick.get_axis(3), 0

        time.sleep(0.02)


def shutdown(thread_joystick_listener):
    stop_event.set()
    if thread_joystick_listener is not None:
        thread_joystick_listener.join(timeout=1.0)
    try:
        client.set_fsm_state(2)
    except Exception:
        pass
    pygame.quit()
    client.close()


def torch_quat_rotate_inverse(q, v):
    """
    Rotate a vector (tensor) by the inverse of a quaternion (tensor).

    :param q: A quaternion tensor in the form of [x, y, z, w] in shape of [N, 4].
    :param v: A vector tensor in the form of [x, y, z] in shape of [N, 3].
    :return: The rotated vector tensor in shape of [N, 3].
    """
    q_w = q[:, -1:]
    q_vec = q[:, :3]

    # Compute the dot product of q_vec and v
    q_vec_dot_v = torch.bmm(q_vec.view(-1, 1, 3), v.view(-1, 3, 1)).squeeze(-1)

    # Compute the cross product of q_vec and v
    q_vec_cross_v = torch.cross(q_vec, v, dim=-1)

    # Compute the rotated vector
    a = v * (2.0 * q_w ** 2 - 1.0)
    b = q_vec_cross_v * q_w * 2.0
    c = q_vec * q_vec_dot_v * 2.0

    return a - b + c


def wrap_to_pi(angle):
    return (angle + numpy.pi) % (2.0 * numpy.pi) - numpy.pi


def quat_xyzw_to_yaw(quat_xyzw):
    x, y, z, w = quat_xyzw
    siny_cosp = 2.0 * (w * z + x * y)
    cosy_cosp = 1.0 - 2.0 * (y * y + z * z)
    return numpy.arctan2(siny_cosp, cosy_cosp)


def main():

    # ============================================================
    # 1.  Global variable initialization
    # ============================================================
    global policy_file_path, policy_model, joystick, axis_left, axis_right, commands_filtered
    global term_hist, vel_cmd_scaled, height_attitude
    global is_walk_flag, is_walk_flag_last, rising_edge_length, falling_edge_length
    global yaw_des, yaw_err
    joystick = None
    axis_left = (0.0, 0.0)
    axis_right = (0.0, 0.0)
    commands_filtered = numpy.array([0.0, 0.0, 0.0])
    vel_cmd_scaled = numpy.array([0.0, 0.0, 0.0], dtype=float)
    height_attitude = HA_INIT.copy()
    term_hist = None
    is_walk_flag = 0.0
    is_walk_flag_last = 0.0
    rising_edge_length = 0
    falling_edge_length = 0
    yaw_des = 0.0
    yaw_err = 0.0


    # ============================================================
    # 2. Initialize joystick and start listener thread
    # ============================================================
    pygame.init()
    pygame.joystick.init()

    joystick_count = pygame.joystick.get_count()
    if joystick_count == 0:
        # No joystick detected -> exit the program
        print("❌ Error: A joystick is required")
        pygame.quit()
        client.close()
        sys.exit(1)
    else:
        print(f"✅ Detected {joystick_count} joystick(s)")
        joystick = pygame.joystick.Joystick(0)
        joystick.init()

    thread_joystick_listener = threading.Thread(target=joystick_listener)
    thread_joystick_listener.start()

    # ============================================================
    # 3. Switch FSM states
    # ============================================================

    # Switch to PD-Stand mode (FSM state 2)
    input("Please Press Enter to switch to FSM state 2 (PDstand)")
    client.set_fsm_state(2)

    # After robot stabilizes, switch to user command task mode (FSM state 10)
    input("After the robot is standing firmly on the ground, Press Enter to switch to FSM state 10 (UserController_A) ")
    client.set_fsm_state(10)
    time.sleep(0.5)


    # ============================================================
    # 4. Motor parameter configuration
    # ============================================================

    kp_config = {
        "left_leg": [200, 200, 120, 200, 115.4, 15],
        "right_leg": [200, 200, 120, 200, 115.4, 15],
        "waist": [300, 450, 300],
        "head": [100, 100],
        "left_manipulator": [200, 200, 50, 200, 30, 30, 30],
        "right_manipulator": [200, 200, 50, 200, 30, 30, 30],
    }

    kd_config = {
        "left_leg": [20, 20, 12, 20, 11.54, 1.5],
        "right_leg": [20, 20, 12, 20, 11.54, 1.5],
        "waist": [20, 30, 20],
        "head": [8, 8],
        "left_manipulator": [10, 10, 2, 10, 0.5, 0.5, 0.5],
        "right_manipulator": [10, 10, 2, 10, 0.5, 0.5, 0.5],
    }
    client.set_motor_cfg_pd(kp_config, kd_config)


    # ============================================================
    # 5. Load policy model and start control loop
    # ============================================================
    policy_file_path = os.path.join(
        os.path.dirname(os.path.abspath(__file__)),
        "wbc_yaw.pt",
    )

    if not os.path.exists(policy_file_path):
        raise FileNotFoundError(f"Policy file not found: {policy_file_path}")

    policy_model = torch.jit.load(policy_file_path, map_location=torch.device('cpu'))
    
    print(f"✅ Policy model loaded from {policy_file_path}")

    # Configure robot control frequency
    target_control_frequency = int(1.0 / CTRL_DT)  # Control loop frequency: 50 Hz
    target_control_period_in_s = CTRL_DT  # Control loop period
    schedule(algorithm, interval=target_control_period_in_s)

    try:
        run_loop()
    except KeyboardInterrupt:
        pass
    finally:
        shutdown(thread_joystick_listener)


if __name__ == "__main__":
    main()
