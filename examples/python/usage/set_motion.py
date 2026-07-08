#!/usr/bin/env python3
from __future__ import annotations

import time

from example_common import close_client, install_stop_signal_handlers, make_parser, print_service_result, wait_for_enter
from fourier_aurora_client import AuroraClient


def request_state_with_confirmation(client: AuroraClient, state_id: int, state_name: str) -> bool:
    if not wait_for_enter(f"Press Enter to switch to {state_name} ({state_id})..."):
        return False
    result = client.requestFsmState(state_id)
    print(f"requestFsmState({state_id}) ", end="")
    print_service_result(result)
    return wait_for_enter(f"Press Enter after confirming {state_name} is active...")


def main() -> int:
    parser = make_parser("python_set_motion", "Switch states and send stand pose / velocity commands.")
    args = parser.parse_args()
    install_stop_signal_handlers()

    client = None
    exit_code = 0
    try:
        client = AuroraClient.get_instance(domain_id=args.domain_id, user_id=args.user_id)
        time.sleep(1.0)

        if not request_state_with_confirmation(client, 13, "PdStandStanding"):
            exit_code = 1
            return 1

        print("set_stand_pose(delta_z=-0.1, delta_pitch=0.0, delta_yaw=0.0, duration=1.0)")
        client.set_stand_pose(delta_z=-0.1, delta_pitch=0.0, delta_yaw=0.0, duration=1.0)
        time.sleep(1.0)

        print("set_stand_pose(delta_z=0.0, delta_pitch=0.0, delta_yaw=0.0, duration=1.0)")
        client.set_stand_pose(delta_z=0.0, delta_pitch=0.0, delta_yaw=0.0, duration=1.0)

        if not request_state_with_confirmation(client, 3, "Walk"):
            exit_code = 1
            return 1

        print("set_velocity(vx=0.3, vy=0.0, yaw=0.0, duration=5.0)")
        client.set_velocity(vx=0.3, vy=0.0, yaw=0.0, duration=5.0)
        time.sleep(5.0)
        return 0
    except KeyboardInterrupt:
        return 0
    except Exception as error:  # noqa: BLE001
        print(f"set_motion failed: {error}")
        exit_code = 1
        return 1
    finally:
        close_client(client, exit_code)


if __name__ == "__main__":
    raise SystemExit(main())
