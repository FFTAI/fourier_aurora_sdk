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

client = AuroraClient.get_instance(domain_id=160, robot_name="Fourier_N1", serial_number=None)


def demo_task():
    # 设置机器人算法频率
    control_frequency = 1  # 机器人控制频率, 1Hz
    control_period = 1.0 / control_frequency  # 机器人控制周期

    # 设置定时任务
    schedule(schedule_task, interval=control_period)

    run_loop()


def schedule_task():
    """
    Update and print state
    """

    """
    Robot States:
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


    # --------------------------------------------------

    # robot_num_of_joints = 23

    # parse state
    imu_quat = client.get_base_data('quat_xyzw')
    imu_euler_angle = client.get_base_data('rpy')
    imu_angular_velocity = client.get_base_data('omega_B')
    imu_acceleration = client.get_base_data('acc_B')

    group_names = ['left_leg', 'right_leg', 'waist', 'left_manipulator', 'right_manipulator']
    joint_position = []
    joint_velocity = []
    joint_torque = []
    for name in group_names:
        joint_position.extend(client.get_group_state(name, 'position'))
        joint_velocity.extend(client.get_group_state(name,'velocity'))
        joint_torque.extend(client.get_group_state(name, 'effort'))

    # group_position_dict = {name: client.get_group_state(name, 'position') for name in group_names}
    # group_velocity_dict = {name: client.get_group_state(name, 'velocity') for name in group_names}
    # group_torque_dict = {name: client.get_group_state(name, 'effort') for name in group_names}

    print("imu_quat = \n", numpy.round(imu_quat, 3))
    print("imu_euler_angle = \n", numpy.round(imu_euler_angle, 3))
    print("imu_angular_velocity = \n", numpy.round(imu_angular_velocity, 3))
    print("imu_acceleration = \n", numpy.round(imu_acceleration, 3))
    print("joint_position = \n", numpy.round(joint_position, 3))
    print("joint_velocity = \n", numpy.round(joint_velocity, 3))
    print("joint_torque = \n", numpy.round(joint_torque, 3))


    # print state
    print("#################################################")


    # --------------------------------------------------


if __name__ == "__main__":
    demo_task()
