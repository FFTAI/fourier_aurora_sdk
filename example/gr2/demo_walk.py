"""

提示：

这个demo要求连接摇杆, 用摇杆控制机器人走路速度。

这个demo仅作为接口调用示例, 提供的策略并不保证机器人能稳定走路，上机运行时请务必做好安全防护措施。 

"""

import time 
from fourier_aurora_client import AuroraClient
import os
import numpy
import torch
import threading


import pygame
from ischedule import run_loop, schedule



# 初始化client
client = AuroraClient.get_instance(domain_id=199, robot_name="gr2t2v2", serial_number=None)
time.sleep(1)

policy_file_path = None
policy_model = None
policy_action = None
obs_buf_stack = None

# ------------------ 控制算法 ------------------
def algorithm():
    global policy_model, policy_action, obs_buf_stack, commands_filtered

    # ============================================================
    # 1. 常量定义
    # ============================================================

    robot_num_of_joints = 29
    policy_control_num_of_joints = 13 # left leg + right leg + waist
    policy_control_index_of_joints = numpy.array([
        0, 1, 2, 3, 4, 5,  # left leg
        6, 7, 8, 9, 10, 11,  # right leg
        12,  # waist
    ])

    default_joint_position = numpy.array([
        # left leg
        -0.1309, 0.0, 0.0, 0.2618, -0.1309, 0,
        # right leg
        -0.1309, 0.0, 0.0, 0.2618, -0.1309, 0,
        # waist
        0.0
    ])
    gravity_vector = numpy.array([
        0.0, 0.0, -1.0
    ])
    action_min = numpy.array([
        -2.6180, -0.5934, -0.6981, -0.0873, -0.7854, -0.38397,  # left leg
        -2.6180, -1.5708, -1.5708, -0.0873, -0.7854, -0.38397,  # right leg
        -2.6180,  # waist
    ]) 
    action_clip_min = \
        action_min \
        - numpy.array([
            1.0, 1.0, 1.0, 1.0, 1.0, 1.0,  # left leg
            1.0, 1.0, 1.0, 1.0, 1.0, 1.0,  # right leg
            1.0,  # waist
        ])
    action_max = numpy.array([
            2.6180, 1.5708, 1.5708, 2.3562, 0.7854, 0.38397,  # left leg
            2.6180, 0.5934, 0.6981, 2.3562, 0.7854, 0.38397,  # right leg
            2.6180,  # waist
    ])

    action_clip_max = \
        action_max \
        + numpy.array([
            1.0, 1.0, 1.0, 1.0, 1.0, 1.0,  # left leg
            1.0, 1.0, 1.0, 1.0, 1.0, 1.0,  # right leg
            1.0,  # waist
        ])


    # ============================================================
    # 2. 读取机器人状态
    # ============================================================

    imu_measured_quat = client.get_base_data('quat_xyzw')
    imu_measured_angular_velocity = client.get_base_data('omega_B') 
    group_names = ['left_leg', 'right_leg', 'waist']
    joint_measured_position = []
    joint_measured_velocity = []  
    for name in group_names:
        joint_measured_position.append(client.get_group_state(name, 'position'))
        joint_measured_velocity.append(client.get_group_state(name, 'velocity'))


    joint_measured_position = numpy.concatenate(joint_measured_position)
    joint_measured_velocity = numpy.concatenate(joint_measured_velocity)


    # ============================================================
    # 3. 生成速度指令 [lin_vel_x, lin_vel_y, ang_vel_yaw], unit: m/s, m/s, rad/s
    #    (来自摇杆)
    # ============================================================

    commands_safety_ratio = 0.9
    command_linear_velocity_x_range = torch.tensor(numpy.array([[-0.80, 0.80]]), dtype=torch.float) \
                                            * commands_safety_ratio
    command_linear_velocity_y_range = torch.tensor(numpy.array([[-1, 1]]), dtype=torch.float) \
                                            * commands_safety_ratio
    command_angular_velocity_yaw_range = torch.tensor(numpy.array([[-1.00, 1.00]]), dtype=torch.float) \
                                                * commands_safety_ratio

    commands_norm = \
        numpy.array([
            -1 * axis_left[1],
            -1 * axis_left[0],
            -1 * axis_right[0],
        ])

    commands = \
        numpy.array([
            commands_norm[0] * numpy.abs(command_linear_velocity_x_range[0, 1].numpy())
            if commands_norm[0] >= 0 else
            commands_norm[0] * numpy.abs(command_linear_velocity_x_range[0, 0].numpy()),

            commands_norm[1] * numpy.abs(command_linear_velocity_y_range[0, 1].numpy())
            if commands_norm[1] >= 0 else
            commands_norm[1] * numpy.abs(command_linear_velocity_y_range[0, 0].numpy()),

            commands_norm[2] * numpy.abs(command_angular_velocity_yaw_range[0, 1].numpy())
            if commands_norm[2] >= 0 else
            commands_norm[2] * numpy.abs(command_angular_velocity_yaw_range[0, 0].numpy())
        ])
    

    # 设置不同的滤波系数
    alpha_x = 0.98      # 对 x 方向速度的滤波系数
    alpha_y = 0.4      # 对 y 方向速度的滤波系数
    alpha_yaw = 0.0    # 对 yaw 角速度的滤波系数

    commands_filtered[0] = commands_filtered[0] * alpha_x + commands[0] * (1 - alpha_x)
    commands_filtered[1] = commands_filtered[1] * alpha_y + commands[1] * (1 - alpha_y)
    commands_filtered[2] = commands_filtered[2] * alpha_yaw + commands[2] * (1 - alpha_yaw)

    print("commands: ", commands_filtered)

    # ============================================================
    # 4. 构造观测量 (obs buffer)
    # ============================================================
    base_measured_quat = numpy.array([0.0, 0.0, 0.0, 1.0, ])
    base_measured_angular_velocity = numpy.array([0.0, 0.0, 0.0, ])

    for i in range(4):
        base_measured_quat[i] = imu_measured_quat[i]

    for i in range(3):
        base_measured_angular_velocity[i] = imu_measured_angular_velocity[i]

    joint_measured_position_for_policy = numpy.zeros(policy_control_num_of_joints)
    joint_measured_velocity_for_policy = numpy.zeros(policy_control_num_of_joints)

    for i in range(policy_control_num_of_joints):
        index = policy_control_index_of_joints[i]
        joint_measured_position_for_policy[i] = joint_measured_position[index]
        joint_measured_velocity_for_policy[i] = joint_measured_velocity[index]

    if policy_action is None:
        policy_action = numpy.zeros(policy_control_num_of_joints)

    # 转换为 torch
    torch_commands = torch.from_numpy(commands_filtered).float().unsqueeze(0)
    torch_base_measured_quat = torch.from_numpy(base_measured_quat).float().unsqueeze(0)
    torch_base_measured_angular_velocity = torch.from_numpy(base_measured_angular_velocity).float().unsqueeze(0)
    torch_joint_measured_position_for_policy = torch.from_numpy(joint_measured_position_for_policy).float().unsqueeze(0)
    torch_joint_measured_velocity_for_policy = torch.from_numpy(joint_measured_velocity_for_policy).float().unsqueeze(0)
    torch_default_joint_position = torch.from_numpy(default_joint_position).float().unsqueeze(0)

    torch_gravity_vector = torch.from_numpy(gravity_vector).float().unsqueeze(0)
    torch_base_project_gravity = torch_quat_rotate_inverse(torch_base_measured_quat, torch_gravity_vector)
    torch_measured_position_offset_for_policy = torch_joint_measured_position_for_policy \
                                                - torch_default_joint_position
    torch_action = torch.from_numpy(policy_action).float().unsqueeze(0)

    obs_buf = torch.cat([
        torch_commands,
        torch_base_measured_angular_velocity,
        torch_base_project_gravity,

        torch_measured_position_offset_for_policy,
        torch_joint_measured_velocity_for_policy * 0.1,
        torch_action,
    ], dim=-1)

    obs_len = obs_buf.shape[-1]
    stack_size = 5

    if obs_buf_stack is None:
        obs_buf_stack = torch.cat([obs_buf] * stack_size, dim=1).float()

    obs_buf_stack = torch.cat([
        obs_buf_stack[:, obs_len:],
        obs_buf,
    ], dim=1).float()


    # ============================================================
    # 5. 策略推理 + 动作生成
    # ============================================================

    torch_policy_action = policy_model(obs_buf_stack).detach()
    torch_policy_action = torch.clip(
        torch_policy_action,
        min=torch.from_numpy(action_clip_min).float().unsqueeze(0),
        max=torch.from_numpy(action_clip_max).float().unsqueeze(0),
    )
    # 记录上一次的 action
    policy_action = torch_policy_action.float().numpy().squeeze(0)
    torch_joint_target_position_from_policy = torch_policy_action \
                                                + torch_default_joint_position
    joint_target_position_from_policy = torch_joint_target_position_from_policy.numpy().squeeze(0)  # unit : rad
    joint_target_position = numpy.zeros(robot_num_of_joints)

    for i in range(policy_control_num_of_joints):
        index = policy_control_index_of_joints[i]
        joint_target_position[index] = joint_target_position_from_policy[i]
        
    print(joint_target_position)

    # ============================================================
    # 6. 下发控制指令
    # ============================================================
    whole_body_pos = {"whole_body": joint_target_position}
    client.set_joint_positions(whole_body_pos)

# ------------------ 监听摇杆输入 ------------------

def joystick_listener():
    global joystick, axis_left, axis_right

    while True:
        pygame.event.get()

        # 获取摇杆输入
        axis_left = joystick.get_axis(0), joystick.get_axis(1)
        axis_right = joystick.get_axis(3), 0

        # 等待 0.02s, 以减少 CPU 占用
        time.sleep(0.02)


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


def main():

    # ============================================================
    # 1. 切换 FSM 状态
    # ============================================================
    # 切换到站立模式 PD stand (FSM state 2)
    input("Please Press Enter to switch to FSM state 2 (PDstand)")
    client.set_fsm_state(2)

    # 机器人站稳后, 切换到用户命令任务模式 (FSM state 10)
    input("After the robot is standing firmly on the ground, Please press Enter to switch to FSM state 10 (User command task) ")
    client.set_fsm_state(10)
    time.sleep(0.5)

    # ============================================================
    # 2. 全局变量初始化
    # ============================================================
    global policy_file_path, policy_model, joystick, axis_left, axis_right, commands_filtered
    joystick = None
    axis_left = (0.0, 0.0)
    axis_right = (0.0, 0.0)
    commands_filtered = numpy.array([0.0, 0.0, 0.0])


    # ============================================================
    # 3. 初始化摇杆并开启监听线程
    # ============================================================
    pygame.init()
    pygame.joystick.init()

    joystick = pygame.joystick.Joystick(0)
    joystick.init()

    thread_joystick_listener = threading.Thread(target=joystick_listener)
    thread_joystick_listener.start()

    # ============================================================
    # 4. 电机参数配置
    # ============================================================

    kp_config = {
        "left_leg": [180, 180, 120, 180, 40, 20],
        "right_leg": [180, 180, 120, 180, 40, 20],
        "waist": [40],
        "head": [20, 20],
        "left_manipulator": [90, 90, 60, 60, 40, 20, 20],
        "right_manipulator": [90, 90, 60, 60, 40, 20, 20]
    }

    kd_config = {
        "left_leg": [21, 10, 8, 21, 5, 2],
        "right_leg": [21, 10, 8, 21, 5, 2],
        "waist": [5],
        "head": [2.5, 2.5],
        "left_manipulator": [10, 10, 5, 5, 2.5, 2.5, 2.5],
        "right_manipulator": [10, 10, 5, 5, 2.5, 2.5, 2.5]
    }
    client.set_motor_cfg(kp_config, kd_config)


    # ============================================================
    # 5. 加载策略模型 + 设置控制循环
    # ============================================================
    policy_file_path = os.path.join(
        os.path.dirname(os.path.abspath(__file__)),
        "policy_jit.pt",
    )
    policy_model = torch.jit.load(policy_file_path, map_location=torch.device('cpu'))

    # 设置机器人算法频率
    target_control_frequency = 50  # 机器人控制频率, 50Hz
    target_control_period_in_s = 1.0 / target_control_frequency  # 机器人控制周期
    schedule(algorithm, interval=target_control_period_in_s)

    run_loop()

if __name__ == "__main__":
    main()
    
