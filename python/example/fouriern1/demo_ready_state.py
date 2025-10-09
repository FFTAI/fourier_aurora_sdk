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

"""

import numpy
from ischedule import run_loop, schedule
from fourier_aurora_client import AuroraClient


# Initialize client
client = AuroraClient.get_instance(domain_id=160, robot_name="Fourier_N1", serial_number=None)

# 切换到fsm state 10 (User command task)
state = input("Please switch to FSM state 10 (User command task)\nPlease enter the desired state number: ")
while not state.isdigit() or int(state) > 11:
    state = input("State number not available, please enter a valid state number: ")
client.set_fsm_state(int(state))

move_count = 0
move_period = 100
joint_start_position = None
kp_config = {
    "left_leg": [180.0, 120.0, 90.0, 120.0, 45.0, 45.0],
    "right_leg": [180.0, 120.0, 90.0, 120.0, 45.0, 45.0],
    "waist": [90.0],
    "left_manipulator": [90.0, 45.0, 45.0, 45.0, 45.0],
    "right_manipulator": [90.0, 45.0, 45.0, 45.0, 45.0]
}

kd_config = {
    "left_leg": [10.0, 10.0, 8.0, 8.0, 2.5, 2.5],
    "right_leg": [10.0, 10.0, 8.0, 8.0, 2.5, 2.5],
    "waist": [8.0],
    "left_manipulator": [ 8.0, 2.5, 2.5, 2.5, 2.5],
    "right_manipulator": [8.0, 2.5, 2.5, 2.5, 2.5]
}


group_names = ['left_leg', 'right_leg', 'waist', 'left_manipulator', 'right_manipulator']
group_sizes = [6, 6, 1, 5, 5]  # 每组关节数目


def wholebodytocontrolgroup(wholebody_position):
    result = {}
    idx = 0
    for name, size in zip(group_names, group_sizes):
        result[name] = list(wholebody_position[idx:idx+size])
        idx += size
    return result


def main():
    # 设置机器人算法频率
    target_control_frequency = 50  # 机器人控制频率
    target_control_period_in_s = 1.0 / target_control_frequency  # 机器人控制周期
    client.set_motor_cfg(kp_config, kd_config)

    # 设置定时任务
    schedule(algorithm, interval=target_control_period_in_s)

    run_loop()


def algorithm():
    global move_count, move_period, joint_start_position

    # update state
    """
    state:
    - imu:
      - quat
      - euler angle (rpy) [deg]
      - angular velocity [deg/s]
      - linear acceleration [m/s^2]
    - joint (in urdf):
      - position [deg]
      - velocity [deg/s]
      - torque [Nm]
    """


    joint_measured_position = []
    for name in group_names:
        joint_measured_position.extend(client.get_group_state(name, 'position'))

    if joint_start_position is None:
        joint_start_position = numpy.array(joint_measured_position)

    joint_end_position = \
            numpy.array([
                # left leg
                -0.2468, 0.0, 0.0, 0.5181, 0.0, -0.2408,
                # right leg
                -0.2468, 0.0, 0.0, 0.5181, 0.0, -0.2408,
                # waist
                0.0,
                # left arm
                0.0, 0.0, 0.0, 0.0, 0.0,
                # right arm
                0.0, 0.0, 0.0, 0.0, 0.0,
            ])

    # update move ratio
    move_ratio = min(move_count / move_period, 1)

    # update target position
    joint_target_position = joint_start_position \
                            + (joint_end_position - joint_start_position) * move_ratio
    

    joint_target_dict = wholebodytocontrolgroup(joint_target_position)
    print(joint_target_dict)


    # update count
    move_count += 1

    # print info
    print("move_ratio = ", numpy.round(move_ratio * 100, 1), "%")

    if move_ratio < 1:
        finish_flag = False
    else:
        finish_flag = True

    if finish_flag is True:
        print("ready state finish!")
        exit(0)

    # output control
    client.set_joint_positions(joint_target_dict)

if __name__ == "__main__":
    main()
