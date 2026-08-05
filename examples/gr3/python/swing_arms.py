#!/usr/bin/env python3
"""Move GR3 arms to a center pose, then run two bounded swing periods."""

import math
import sys
import time

from fourier_aurora_client import (
    AuroraClient,
    AuroraResource,
    ControlGroupCommand,
    Endpoint,
    JointCommand,
    JointControlType,
    MotorMode,
)

PD_STAND_STANDING_STATE = 13
SHOULDER_ROLL_INDEX = 1
ELBOW_INDEX = 3
INTERPOLATION_SECONDS = 3.0
SWING_PERIOD_SECONDS = 5.0
SWING_SECONDS = 10.0
CONTROL_PERIOD_SECONDS = 0.01


def print_result(operation, result):
    print(f'{operation} failed: code={result.code} message="{result.message}"')


def find_group(state, name):
    return next(
        (group for group in state.joint_state if group.group_name == name),
        None,
    )


def make_joint_command(group_name, position):
    command = JointCommand()
    command.group_name = group_name
    command.control_type = JointControlType.JOINT
    command.motor_mode = MotorMode.PD
    command.position = position
    command.velocity = [0.0] * len(position)
    command.effort = [0.0] * len(position)
    return command


def publish_arms(client, left, right):
    command = ControlGroupCommand()
    command.joint_cmd = [
        make_joint_command("left_manipulator", left),
        make_joint_command("right_manipulator", right),
    ]
    result = client.publish_control_group_command(command)
    if not result.success():
        print_result("publish_control_group_command", result)
        return False
    return True


def sleep_until(deadline):
    remaining = deadline - time.monotonic()
    if remaining > 0.0:
        time.sleep(remaining)


def interpolate_to_center(
    client,
    left_start,
    right_start,
    left_center,
    right_center,
):
    begin = time.monotonic()
    step = 0
    while True:
        alpha = min(1.0, (time.monotonic() - begin) / INTERPOLATION_SECONDS)
        left = [
            start + alpha * (target - start)
            for start, target in zip(left_start, left_center)
        ]
        right = [
            start + alpha * (target - start)
            for start, target in zip(right_start, right_center)
        ]
        if not publish_arms(client, left, right):
            return False
        if alpha >= 1.0:
            return True
        step += 1
        sleep_until(begin + step * CONTROL_PERIOD_SECONDS)


def swing_arms(client, left_center, right_center):
    begin = time.monotonic()
    step = 0
    while True:
        elapsed = time.monotonic() - begin
        if elapsed >= SWING_SECONDS:
            break
        offset = 0.7 * math.sin(
            2.0 * math.pi * elapsed / SWING_PERIOD_SECONDS
        )
        left = list(left_center)
        right = list(right_center)
        left[ELBOW_INDEX] += offset
        right[ELBOW_INDEX] -= offset
        if not publish_arms(client, left, right):
            return False
        step += 1
        sleep_until(begin + step * CONTROL_PERIOD_SECONDS)
    return publish_arms(client, left_center, right_center)


def main():
    if len(sys.argv) != 2:
        print(f"Usage: {sys.argv[0]} <nonzero_source_id>")
        return 2
    try:
        source_id = int(sys.argv[1])
    except ValueError:
        source_id = 0
    if source_id <= 0 or source_id > 0xFFFFFFFF:
        print(f"Usage: {sys.argv[0]} <nonzero_source_id>")
        return 2

    print(
        "GR3 swing_arms will request PdStandStanding (FSM 13), "
        f"interpolate both arms for {INTERPOLATION_SECONDS} s, then run "
        f"two {SWING_PERIOD_SECONDS} s swing periods. The motion lasts "
        f"about {INTERPOLATION_SECONDS + SWING_SECONDS} s."
    )
    try:
        confirmation = input(
            "Confirm the robot is supported and the area is clear. "
            "Type YES to continue: "
        )
        if confirmation != "YES":
            print("Cancelled.")
            return 0
    except KeyboardInterrupt:
        return 130

    client = AuroraClient.get_instance()
    interrupted = False
    lease_registered = False
    try:
        options = client.config()
        options.source_id = source_id
        options.enabled_endpoints = [
            Endpoint.FSM_SERVICE,
            Endpoint.MANAGE_LEASE_SERVICE,
            Endpoint.AURORA_STATE_SUBSCRIBER,
            Endpoint.CONTROL_GROUP_STATE_SUBSCRIBER,
            Endpoint.CONTROL_GROUP_COMMAND_PUBLISHER,
        ]
        client.configure(options)
        client.start()

        if not client.wait_for_endpoints(5.0):
            print("GR3 arm endpoints were not ready within 5 s.")
            return 1

        lease = client.register_lease(
            [
                AuroraResource.LEFT_MANIPULATOR,
                AuroraResource.RIGHT_MANIPULATOR,
            ]
        )
        if not lease.usable():
            print_result("register_lease(manipulators)", lease.operation)
            return 1
        lease_registered = True

        result = client.request_fsm_state(PD_STAND_STANDING_STATE)
        if not result.success():
            print_result("request_fsm_state(PdStandStanding)", result)
            return 1
        if not client.wait_for_fsm_state(PD_STAND_STANDING_STATE, 10.0):
            print(
                "GR3 did not enter PdStandStanding (FSM 13) within 10 s."
            )
            return 1

        result, state = client.get_control_group_state()
        if not result.success():
            print_result("get_control_group_state", result)
            return 1

        # Both starts come from this one state snapshot.
        left = find_group(state, "left_manipulator")
        right = find_group(state, "right_manipulator")
        if (
            left is None
            or right is None
            or len(left.joint_position) <= ELBOW_INDEX
            or len(right.joint_position) <= ELBOW_INDEX
        ):
            print(
                "Both GR3 manipulator groups must contain shoulder-roll "
                "index 1 and elbow index 3."
            )
            return 1

        left_center = [0.0] * len(left.joint_position)
        right_center = [0.0] * len(right.joint_position)
        left_center[SHOULDER_ROLL_INDEX] = 0.2
        right_center[SHOULDER_ROLL_INDEX] = -0.2
        left_center[ELBOW_INDEX] = -0.7
        right_center[ELBOW_INDEX] = -0.7

        if not interpolate_to_center(
            client,
            list(left.joint_position),
            list(right.joint_position),
            left_center,
            right_center,
        ):
            return 1
        if not swing_arms(client, left_center, right_center):
            return 1
        return 0
    except KeyboardInterrupt:
        interrupted = True
        return 130
    except Exception as error:  # noqa: BLE001
        print(f"GR3 swing_arms failed: {error}")
        return 1
    finally:
        try:
            if lease_registered:
                release = client.release_lease()
                if not release.success():
                    print_result("release_lease", release.operation)
            client.stop()
        except Exception as error:  # noqa: BLE001
            print(f"stop failed: {error}")
            if not interrupted:
                raise


if __name__ == "__main__":
    raise SystemExit(main())
