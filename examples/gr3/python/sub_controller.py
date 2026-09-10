#!/usr/bin/env python3
"""Exercise GR3 arm passthrough and Universal Controller rate updates."""

import sys
import time

from fourier_aurora_client import (
    AuroraClient,
    AuroraResource,
    ControlGroupCommand,
    ControllerParam,
    Endpoint,
    JointCommand,
    JointControlType,
    MotorMode,
    SubControllerCommand,
)

EXTERNAL_UNICONTROLLER_STATE = 120
ARM_JOINT_COUNT = 7
ELBOW_INDEX = 3
INTERPOLATION_SECONDS = 2.0
CONTROL_PERIOD_SECONDS = 0.01


def print_result(operation, result):
    print(f'{operation} failed: code={result.code} message="{result.message}"')


def make_controller_param(name, rate):
    param = ControllerParam()
    param.controller_name = name
    param.rate = rate
    return param


def set_controller_rates(client, momentum_rate, waist_rate):
    command = SubControllerCommand()
    command.controller_params = [
        make_controller_param("momentum_compensation", momentum_rate),
        make_controller_param("waist_compensation", waist_rate),
    ]
    return client.set_sub_controller(command)


def make_joint_command(group_name, position):
    command = JointCommand()
    command.group_name = group_name
    command.control_type = JointControlType.JOINT
    command.motor_mode = MotorMode.PD
    command.position = position
    command.velocity = [0.0] * len(position)
    command.effort = [0.0] * len(position)
    return command


def find_group(state, name):
    return next(
        (group for group in state.joint_state if group.group_name == name),
        None,
    )


def publish_arms(client, left_position, right_position):
    command = ControlGroupCommand()
    command.joint_cmd = [
        make_joint_command("left_manipulator", left_position),
        make_joint_command("right_manipulator", right_position),
    ]
    return client.publish_control_group_command(command)


def sleep_until(deadline):
    remaining = deadline - time.monotonic()
    if remaining > 0.0:
        time.sleep(remaining)


def move_arms(client, elbow_position):
    result, state = client.get_control_group_state()
    if not result.success():
        return result

    left = find_group(state, "left_manipulator")
    right = find_group(state, "right_manipulator")
    if (
        left is None
        or right is None
        or len(left.joint_position) != ARM_JOINT_COUNT
        or len(right.joint_position) != ARM_JOINT_COUNT
    ):
        raise RuntimeError(
            "both GR3 manipulator groups must contain seven joints"
        )

    target = [0.0] * ARM_JOINT_COUNT
    if elbow_position is not None:
        target[ELBOW_INDEX] = elbow_position

    left_start = list(left.joint_position)
    right_start = list(right.joint_position)
    begin = time.monotonic()
    step = 0
    while True:
        alpha = min(
            1.0,
            (time.monotonic() - begin) / INTERPOLATION_SECONDS,
        )
        left_position = [
            start + alpha * (target_value - start)
            for start, target_value in zip(left_start, target)
        ]
        right_position = [
            start + alpha * (target_value - start)
            for start, target_value in zip(right_start, target)
        ]
        result = publish_arms(client, left_position, right_position)
        if not result.success() or alpha >= 1.0:
            return result
        step += 1
        sleep_until(begin + step * CONTROL_PERIOD_SECONDS)


def main():
    if len(sys.argv) != 2 or not (
        1 <= len(sys.argv[1]) <= 64
        and all(
            character.isascii()
            and (character.isalnum() or character in "._-")
            for character in sys.argv[1]
        )
    ):
        print(f"Usage: {sys.argv[0]} <source_name>")
        return 2
    source_name = sys.argv[1]

    print(
        "GR3 sub_controller will request ExternalUniController (FSM 120), "
        "command both arms, and change momentum/waist compensation rates. "
        "Make sure the robot is supported and the area is clear."
    )

    client = AuroraClient.get_instance()
    lease_registered = False
    try:
        options = client.config()
        options.domain_id = 89
        options.source_name = source_name
        options.enabled_endpoints = [
            Endpoint.FSM_SERVICE,
            Endpoint.MANAGE_LEASE_SERVICE,
            Endpoint.SUB_CONTROLLER_SERVICE,
            Endpoint.AURORA_STATE_SUBSCRIBER,
            Endpoint.CONTROL_GROUP_STATE_SUBSCRIBER,
            Endpoint.CONTROL_GROUP_COMMAND_PUBLISHER,
        ]
        client.configure(options)
        client.start()

        if not client.wait_for_endpoints(5.0):
            print("GR3 sub-controller endpoints were not ready within 5 s.")
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

        result = client.request_fsm_state(EXTERNAL_UNICONTROLLER_STATE)
        if not result.success():
            print_result("request_fsm_state(ExternalUniController)", result)
            return 1
        if not client.wait_for_fsm_state(EXTERNAL_UNICONTROLLER_STATE, 1.0):
            print("GR3 did not enter ExternalUniController (FSM 120) within 11 s.")
            return 1

        result = set_controller_rates(client, 0.0, 1.0)
        if not result.success():
            print_result("set_controller_rates(0.0, 0.0)", result)
            return 1

        result = move_arms(client, None)
        if not result.success():
            print_result("move_arms(all zero)", result)
            return 1

        input("Arms commanded to zero. Press Enter to continue: ")

        result = move_arms(client, -1.5)
        if not result.success():
            print_result("move_arms(elbows=-1.5)", result)
            return 1

        result = set_controller_rates(client, 0.5, 1.0)
        if not result.success():
            print_result("set_controller_rates(0.5, 1.0)", result)
            return 1

        input(
            "Elbows commanded to -1.0; momentum=0.5 and waist=1.0. "
            "Press Enter to reset: "
        )

        result = client.reset_unicontroller()
        if not result.success():
            print_result("reset_unicontroller", result)
            return 1
        return 0
    except KeyboardInterrupt:
        return 130
    except Exception as error:  # noqa: BLE001
        print(f"GR3 sub_controller failed: {error}")
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


if __name__ == "__main__":
    raise SystemExit(main())
