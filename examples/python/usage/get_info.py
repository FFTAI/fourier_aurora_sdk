#!/usr/bin/env python3
from __future__ import annotations

import time

from example_common import close_client, install_stop_signal_handlers, make_parser, wait_for
from fourier_aurora_client import AuroraClient


def main() -> int:
    parser = make_parser("python_get_info", "Print Aurora state, base data, and left leg joint position.")
    args = parser.parse_args()
    install_stop_signal_handlers()

    client = None
    exit_code = 0
    try:
        client = AuroraClient.get_instance(domain_id=args.domain_id, user_id=args.user_id)
        time.sleep(1.0)

        aurora_state = wait_for(client.getAuroraState, "AuroraState")
        base_data = wait_for(client.getBaseData, "BaseData")
        left_leg_position = wait_for(
            lambda: client.get_group_state("left_leg", "position"),
            "left_leg position",
        )

        while True:
            aurora_state = client.getAuroraState()
            base_data = client.getBaseData()
            left_leg_position = client.get_group_state("left_leg", "position")

            print(f"robot_type: {aurora_state.robot_info.robot_type}")
            print(f"state_id: {aurora_state.current_state.id}")
            print(f"state_name: {aurora_state.current_state.name}")
            print(f"base_rpy: {list(base_data.rpy)}")
            print(f"stable_level: {base_data.stable_level}")
            print(f"left_leg_position: {left_leg_position}")
            print("------------------------------")
            time.sleep(1.0)
    except KeyboardInterrupt:
        print("KeyboardInterrupt received, exiting...")
        return 0
    except Exception as error:  # noqa: BLE001
        print(f"get_info failed: {error}")
        exit_code = 1
        return 1
    finally:
        close_client(client, exit_code)


if __name__ == "__main__":
    raise SystemExit(main())
