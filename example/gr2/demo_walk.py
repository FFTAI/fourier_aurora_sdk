"""
Copyright (C) [2024] [Fourier Intelligence Ltd.]

This program is free software; you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation; either version 3 of the License, or
(at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program; if not, write to the Free Software
Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA

--------------------------------------------------

Demo code for Fourier robots

Run this script by:
    python demo_xxx.py --config=config_xxx.yaml
    - config_xxx.yaml is the configuration file for the Fourier robots

这个 demo 要求连接摇杆, 用摇杆控制机器人走路速度

"""

import time 
from fourier_aurora_client import AuroraClient
import os
import numpy
import torch
import threading


import pygame
from ischedule import run_loop, schedule



# Initialize client
client = AuroraClient.get_instance(domain_id=123, robot_name="gr2t2v2", serial_number=None)
time.sleep(1)

policy_file_path = None
policy_model = None
policy_action = None
obs_buf_stack = None


def main():
    # 切换到Use_cmd Task: 10
    input("Please Press Enter to switch to FSM state 2 (PDstand)")
    client.set_fsm_state(2)

    input("After the robot is standing firmly on the ground, Please press Enter to switch to FSM state 10 (User command task) ")
    client.set_fsm_state(10)
    time.sleep(0.5)
    # state = input("Please switch to FSM state 10 (User command task)\nPlease enter the desired state number: ")
    # while not state.isdigit() or int(state) > 11:
    #     state = input("State number not available, please enter a valid state number: ")
    # client.set_fsm_state(int(state))
    # time.sleep(0.5)

    # 初始化设置
    global policy_file_path, policy_model, joystick, axis_left, axis_right, commands_filtered
    joystick = None
    axis_left = (0.0, 0.0)
    axis_right = (0.0, 0.0)
    commands_filtered = numpy.array([0.0, 0.0, 0.0])

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


####################################################################
    # 监听摇杆输入
    def joystick_listener():
        global joystick, axis_left, axis_right

        while True:
            pygame.event.get()

            # 获取摇杆输入
            axis_left = joystick.get_axis(0), joystick.get_axis(1)
            axis_right = joystick.get_axis(3), 0

            # 等待 0.02s, 以减少 CPU 占用
            time.sleep(0.02)

    pygame.init()
    pygame.joystick.init()

    joystick = pygame.joystick.Joystick(0)
    joystick.init()

    thread_joystick_listener = threading.Thread(target=joystick_listener)
    thread_joystick_listener.start()

    # 等待 1s (确保摇杆监听线程启动)
    # time.sleep(1)

#############################################################################

    # 设置电机参数
    client.set_motor_cfg(kp_config, kd_config)
    # Load Model
    policy_file_path = os.path.join(
        os.path.dirname(os.path.abspath(__file__)),
        "policy_jit.pt",
    )
    policy_model = torch.jit.load(policy_file_path, map_location=torch.device('cpu'))

    # 设置定时任务
    # 设置机器人算法频率
    target_control_frequency = 50  # 机器人控制频率, 50Hz
    target_control_period_in_s = 1.0 / target_control_frequency  # 机器人控制周期
    schedule(algorithm, interval=target_control_period_in_s)

    run_loop()


def algorithm():
    global policy_model, policy_action, obs_buf_stack, commands_filtered

    """
    state:
    - imu:
      - quat (x, y, z, w)
      - euler angle (rpy) [deg]
      - angular velocity [deg/s]
      - linear acceleration [m/s^2]
    - joint (in urdf):
      - position [deg]
      - velocity [deg/s]
      - torque [Nm]
    """
    # --------------------------------------------------

    robot_num_of_joints = 29
    policy_control_num_of_joints = 13 # left leg + right leg + waist
    policy_control_index_of_joints = numpy.array([
        0, 1, 2, 3, 4, 5,  # left leg
        6, 7, 8, 9, 10, 11,  # right leg
        12,  # waist
    ])

    # get states
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
    # joint_measured_position = numpy.array(client.get_group_state('whole_body', 'position'))
    # joint_measured_velocity = numpy.array(client.get_group_state('whole_body','velocity'))


    

    # --------------------------------------------------

    # constants
    default_joint_position = numpy.array([
        # left leg
        -0.1309, 0.0, 0.0, 0.2618, -0.1309, 0,
        # right leg
        -0.1309, 0.0, 0.0, 0.2618, -0.1309, 0,
        # waist
        0.0,
        # # head
        # 0.0, 0.0,
        # # left manipulator
        # 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0,
        # # right manipulator
        # 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0,
    ])
    gravity_vector = numpy.array([
        0.0, 0.0, -1.0
    ])
    action_min = numpy.array([
        -2.6180, -0.5934, -0.6981, -0.0873, -0.7854, -0.38397,  # left leg
        -2.6180, -1.5708, -1.5708, -0.0873, -0.7854, -0.38397,  # right leg
        -2.6180,  # waist
        # -1.3963, -0.5236,  # head
        # -2.9671, -0.5236, -1.8326, -1.5272, -1.8326, -0.6109, -0.9600,  # left arm
        # -2.9671, -2.7925, -1.8326, -1.5272, -1.8326, -0.6109, -0.9600,  # right arm
    ]) 
    action_clip_min = \
        action_min \
        - numpy.array([
            1.0, 1.0, 1.0, 1.0, 1.0, 1.0,  # left leg
            1.0, 1.0, 1.0, 1.0, 1.0, 1.0,  # right leg
            1.0,  # waist
            # 1.0, 1.0,  # head
            # 1.0, 1.0, 1.0, 1.0, 1.0, 1.0, 1.0,  # left arm
            # 1.0, 1.0, 1.0, 1.0, 1.0, 1.0, 1.0,  # right arm
        ])
    action_max = numpy.array([
            2.6180, 1.5708, 1.5708, 2.3562, 0.7854, 0.38397,  # left leg
            2.6180, 0.5934, 0.6981, 2.3562, 0.7854, 0.38397,  # right leg
            2.6180,  # waist
            # 1.3963, 0.5236,  # head
            # 2.9671, 2.7925, 1.8326, 0.4800, 1.8326, 0.6109, 0.9600,  # left arm
            # 2.9671, 0.5236, 1.8326, 0.4800, 1.8326, 0.6109, 0.9600,  # right arm
    ])

    action_clip_max = \
        action_max \
        + numpy.array([
            1.0, 1.0, 1.0, 1.0, 1.0, 1.0,  # left leg
            1.0, 1.0, 1.0, 1.0, 1.0, 1.0,  # right leg
            1.0,  # waist
            # 1.0, 1.0,  # head
            # 1.0, 1.0, 1.0, 1.0, 1.0, 1.0, 1.0,  # left arm
            # 1.0, 1.0, 1.0, 1.0, 1.0, 1.0, 1.0,  # right arm
        ])

    # --------------------------------------------------

    # prepare input

    # 指令速度: 
    # [lin_vel_x, lin_vel_y, ang_vel_yaw], unit: m/s, m/s, rad/s

    commands_safety_ratio = 0.9
    command_linear_velocity_x_range = torch.tensor(numpy.array([[-0.80, 0.80]]), dtype=torch.float) \
                                            * commands_safety_ratio
    command_linear_velocity_y_range = torch.tensor(numpy.array([[-1, 1]]), dtype=torch.float) \
                                            * commands_safety_ratio
    command_angular_velocity_yaw_range = torch.tensor(numpy.array([[-1.00, 1.00]]), dtype=torch.float) \
                                                * commands_safety_ratio
    # commands = numpy.array([0.0, 0.0, 0.0, ])

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
    base_measured_quat = numpy.array([0.0, 0.0, 0.0, 1.0, ])
    base_measured_angular_velocity = numpy.array([0.0, 0.0, 0.0, ])

    for i in range(4):
        base_measured_quat[i] = imu_measured_quat[i]

    for i in range(3):
        base_measured_angular_velocity[i] = imu_measured_angular_velocity[i]

    # joint_measured_position_for_policy = joint_measured_position
    # joint_measured_velocity_for_policy = joint_measured_velocity

    joint_measured_position_for_policy = numpy.zeros(policy_control_num_of_joints)
    joint_measured_velocity_for_policy = numpy.zeros(policy_control_num_of_joints)

    for i in range(policy_control_num_of_joints):
        index = policy_control_index_of_joints[i]
        joint_measured_position_for_policy[i] = joint_measured_position[index]
        joint_measured_velocity_for_policy[i] = joint_measured_velocity[index]

    if policy_action is None:
        policy_action = numpy.zeros(policy_control_num_of_joints)



    # run algorithm
    torch_commands = torch.from_numpy(commands_filtered).float().unsqueeze(0)
    torch_base_measured_quat = torch.from_numpy(base_measured_quat).float().unsqueeze(0)
    torch_base_measured_angular_velocity = torch.from_numpy(base_measured_angular_velocity).float().unsqueeze(0)
    torch_joint_measured_position_for_policy = torch.from_numpy(joint_measured_position_for_policy).float().unsqueeze(0)
    torch_joint_measured_velocity_for_policy = torch.from_numpy(joint_measured_velocity_for_policy).float().unsqueeze(0)
    torch_default_joint_position = torch.from_numpy(default_joint_position).float().unsqueeze(0)

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

    # print("obs_buf_stack: ", obs_buf_stack)

    torch_policy_action = policy_model(obs_buf_stack).detach()
    # print("policy output action: ", torch_policy_action)

    torch_policy_action = torch.clip(
        torch_policy_action,
        min=torch.from_numpy(action_clip_min).float().unsqueeze(0),
        max=torch.from_numpy(action_clip_max).float().unsqueeze(0),
    )
    # print("clipped policy output action: ", torch_policy_action)
    # 记录上一次的 action
    policy_action = torch_policy_action.float().numpy().squeeze(0)
    # print("action: ", policy_action)

    torch_joint_target_position_from_policy = torch_policy_action \
                                                + torch_default_joint_position

    joint_target_position_from_policy = torch_joint_target_position_from_policy.numpy().squeeze(0)  # unit : rad

    joint_target_position = numpy.zeros(robot_num_of_joints)

    for i in range(policy_control_num_of_joints):
        index = policy_control_index_of_joints[i]
        joint_target_position[index] = joint_target_position_from_policy[i]

    print(joint_target_position)
    whole_body_pos = {"whole_body": joint_target_position}

    print("whole_body_pos: ", whole_body_pos)

    # positions = {
    #     "left_leg": joint_target_position[0:6],
    #     "right_leg": joint_target_position[6:12],
    #     "waist": joint_target_position[12:13],
    #     "head": [0, 0],
    #     "left_manipulator": [0, 0, 0, 0, 0, 0, 0],
    #     "right_manipulator": [0, 0, 0, 0, 0, 0, 0]
    # }

    client.set_joint_positions(whole_body_pos)



if __name__ == "__main__":
    main()
    
