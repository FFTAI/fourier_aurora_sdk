#!/usr/bin/env python3
from __future__ import annotations

import math
import time

from example_common import close_client, install_stop_signal_handlers, make_parser, print_service_result, stop_requested, wait_for_enter
from fourier_aurora_client import AuroraClient


PDSTAND_HANGING = 12
PDSTAND_STANDING = 13
CONTROL_PERIOD = 0.01
INTERPOLATION_STEPS = 300
HOLD_STEPS = 100
SWING_PERIOD_STEPS = 500
SHOULDER_ROLL_INDEX = 1
ELBOW_INDEX = 3


def get_arm_position(client: AuroraClient, group_name: str) -> list[float]:
    try:
        return list(client.get_group_cmd(group_name, "position"))
    except Exception:  # noqa: BLE001
        return list(client.get_group_state(group_name, "position"))


def publish_arms(client: AuroraClient, left_position: list[float], right_position: list[float]) -> None:
    client.set_group_cmd(
        position_cmd={
            "left_manipulator": left_position,
            "right_manipulator": right_position,
        },
        velocity_cmd={
            "left_manipulator": [0.0] * len(left_position),
            "right_manipulator": [0.0] * len(right_position),
        },
        effort_cmd={
            "left_manipulator": [0.0] * len(left_position),
            "right_manipulator": [0.0] * len(right_position),
        },
    )


def main() -> int:
    parser = make_parser("python_set_arm_swing", "Swing both arms in PdStandStanding.")
    args = parser.parse_args()
    install_stop_signal_handlers()

    client = None
    exit_code = 0
    try:
        client = AuroraClient.get_instance(domain_id=args.domain_id, user_id=args.user_id)
        time.sleep(1.0)

        result = client.requestFsmState(PDSTAND_HANGING)
        print(f"requestFsmState({PDSTAND_HANGING}) ", end="")
        print_service_result(result)
        if not result.success():
            print("Continue anyway; the robot may already be in PdStandStanding.")

        if not wait_for_enter(
            "Place the robot onto the ground. Press Enter after it switches to PdStandStanding (13)..."
        ):
            exit_code = 1
            return 1

        aurora_state = client.getAuroraState()
        if aurora_state.current_state.id != PDSTAND_STANDING:
            print(
                "Robot is not in PdStandStanding (13). "
                f"Current state is {aurora_state.current_state.id} ({aurora_state.current_state.name})."
            )
            print("Place the robot onto the ground again and rerun this example.")
            exit_code = 1
            return 1

        left_start = get_arm_position(client, "left_manipulator")
        right_start = get_arm_position(client, "right_manipulator")
        if len(left_start) <= ELBOW_INDEX or len(right_start) <= ELBOW_INDEX:
            print("Arm control groups must contain at least shoulder roll and elbow joints.")
            exit_code = 1
            return 1

        left_target = [0.0] * len(left_start)
        right_target = [0.0] * len(right_start)
        left_target[SHOULDER_ROLL_INDEX] = 0.2
        right_target[SHOULDER_ROLL_INDEX] = -0.2
        left_target[ELBOW_INDEX] = -0.7
        right_target[ELBOW_INDEX] = -0.7

        for step in range(1, INTERPOLATION_STEPS + 1):
            if stop_requested():
                break
            alpha = step / INTERPOLATION_STEPS
            left_position = [
                start + alpha * (target - start)
                for start, target in zip(left_start, left_target)
            ]
            right_position = [
                start + alpha * (target - start)
                for start, target in zip(right_start, right_target)
            ]
            publish_arms(client, left_position, right_position)
            time.sleep(CONTROL_PERIOD)

        for _ in range(HOLD_STEPS):
            if stop_requested():
                break
            publish_arms(client, left_target, right_target)
            time.sleep(CONTROL_PERIOD)

        swing_step = 0
        while not stop_requested():
            theta = 2.0 * math.pi * (swing_step % SWING_PERIOD_STEPS) / SWING_PERIOD_STEPS
            left_position = list(left_target)
            right_position = list(right_target)
            left_position[ELBOW_INDEX] = -0.7 + 0.7 * math.sin(theta)
            right_position[ELBOW_INDEX] = -0.7 + 0.7 * math.sin(theta + math.pi)
            publish_arms(client, left_position, right_position)
            swing_step += 1
            time.sleep(CONTROL_PERIOD)

        return 0
    except KeyboardInterrupt:
        return 0
    except Exception as error:  # noqa: BLE001
        print(f"set_arm_swing failed: {error}")
        exit_code = 1
        return 1
    finally:
        close_client(client, exit_code)


if __name__ == "__main__":
    raise SystemExit(main())
