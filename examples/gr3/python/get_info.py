#!/usr/bin/env python3
"""Read one GR3 robot, estimator, and control-group snapshot."""

from fourier_aurora_client import AuroraClient, Endpoint


def print_result(operation, result):
    print(f'{operation} failed: code={result.code} message="{result.message}"')


def main():
    client = AuroraClient.get_instance()
    try:
        options = client.config()
        options.enabled_endpoints = [
            Endpoint.AURORA_STATE_SUBSCRIBER,
            Endpoint.ESTIMATOR_DATA_SUBSCRIBER,
            Endpoint.CONTROL_GROUP_STATE_SUBSCRIBER,
        ]
        client.configure(options)
        client.start()

        if not client.wait_for_endpoints(5.0):
            print("GR3 information endpoints were not ready within 5 s.")
            return 1

        result, aurora_state = client.get_aurora_state()
        if not result.success():
            print_result("get_aurora_state", result)
            return 1

        result, estimator = client.get_estimator_data()
        if not result.success():
            print_result("get_estimator_data", result)
            return 1

        result, control_groups = client.get_control_group_state()
        if not result.success():
            print_result("get_control_group_state", result)
            return 1

        # Aurora SDK numeric state uses SI units unless a field says otherwise.
        print(f"robot_type: {aurora_state.robot_info.robot_type}")
        print(f"hardware_type: {aurora_state.robot_info.hardware_type}")
        print(
            "end_effector_type: "
            f"{aurora_state.robot_info.end_effector_type}"
        )
        print(
            f"fsm_state: {aurora_state.current_state.name} "
            f"({aurora_state.current_state.id})"
        )
        print(f"base_rpy_rad: {estimator.rpy}")
        print(f"base_velocity_world_mps: {estimator.vel_W}")
        print(f"stable_level: {estimator.stable_level}")
        print(
            "control_groups: "
            + " ".join(group.group_name for group in control_groups.joint_state)
        )
        return 0
    except Exception as error:  # noqa: BLE001
        print(f"GR3 get_info failed: {error}")
        return 1
    finally:
        try:
            client.stop()
        except Exception as error:  # noqa: BLE001
            print(f"stop failed: {error}")


if __name__ == "__main__":
    raise SystemExit(main())
