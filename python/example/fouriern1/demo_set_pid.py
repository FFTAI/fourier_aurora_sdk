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

import time 
from ischedule import run_loop, schedule

from fourier_aurora_client import AuroraClient

client = AuroraClient.get_instance(domain_id=160, robot_name="Fourier_N1", serial_number=None)


def main():

    # 切换到fsm state 10 (User command task)
    state = input("Please switch to FSM state 10 (User command task)\nPlease enter the desired state number: ")
    while not state.isdigit() or int(state) > 11:
        state = input("State number not available, please enter a valid state number: ")
    client.set_fsm_state(int(state))
    time.sleep(0.5)


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
        "left_manipulator": [8.0, 2.5, 2.5, 2.5, 2.5],
        "right_manipulator": [8.0, 2.5, 2.5, 2.5, 2.5]
    }

    client.set_motor_cfg(kp_config, kd_config)

    client.close()
    print("Motor configuration set successfully.")

if __name__ == "__main__":
    main()
