#!/usr/bin/env python3
"""SDK 1.0.1 teaching example. Default is read-only; never changes the FSM."""
import argparse
import math
import sys
import time


def require(result):
    if not result.success():
        raise RuntimeError(f"{result.code}: {result.message}")


def read_first(getter):
    deadline = time.monotonic() + 3.0
    while True:
        result, value = getter()
        if result.success():
            return value
        if time.monotonic() >= deadline:
            require(result)
        time.sleep(0.05)


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("action", choices=["read", "walk", "head"], nargs="?", default="read")
    parser.add_argument("--domain", type=int, default=123)
    parser.add_argument("--namespace")
    parser.add_argument("--source", default="mini-doc-example")
    args = parser.parse_args()
    if args.action != "read":
        print("Motion example: verify support, clear the area, and have the hardware stop available.")
        print("walk: 0.1 m/s for 2 s in an existing walking state; head: yaw +0.03 rad and back in 201.")
        if input("Type YES to run: ") != "YES":
            return 0

    from fourier_aurora_client import (
        AuroraClient, AuroraResource, Endpoint, ControlGroupCommand,
        JointCommand, JointControlType, MotorMode,
    )
    client = AuroraClient.get_instance()
    acquired = False
    motion_sent = False
    exit_code = 0
    try:
        options = client.config()
        options.domain_id = args.domain
        options.source_name = args.source
        if args.namespace is not None:
            options.namespace_name = args.namespace
        options.enabled_endpoints = [Endpoint.AURORA_STATE_SUBSCRIBER]
        if args.action != "read":
            options.enabled_endpoints += [Endpoint.MANAGE_LEASE_SERVICE]
            options.enabled_endpoints += (
                [Endpoint.MOTION_COMMAND_PUBLISHER] if args.action == "walk" else
                [Endpoint.CONTROL_GROUP_COMMAND_PUBLISHER, Endpoint.CONTROL_GROUP_STATE_SUBSCRIBER]
            )
        client.configure(options)
        client.start()
        if not client.wait_for_endpoints(5.0):
            raise RuntimeError("Required endpoints did not match")
        state = read_first(client.get_aurora_state)
        print(state.robot_info.robot_type, state.robot_info.hardware_type)
        print(state.current_state.id, state.current_state.name)
        if args.action == "read":
            for entry in state.available_states:
                print(entry.id, entry.name)
        else:
            if (state.robot_info.robot_type, state.robot_info.hardware_type) != ("gr3mini", "v3_0_0"):
                raise RuntimeError("This motion example requires gr3mini v3_0_0")
            allowed = {110302, 110311} if args.action == "walk" else {201}
            initial_state = state.current_state.id
            if initial_state not in allowed:
                raise RuntimeError(f"Prepare one of these states first: {sorted(allowed)}")
            resource = AuroraResource.MOTION_COMMAND if args.action == "walk" else AuroraResource.HEAD
            lease = client.register_lease([resource])
            if not lease.usable():
                raise RuntimeError(f"Lease unavailable: {lease.operation.code}: {lease.operation.message}")
            acquired = True
            start_position = None
            if args.action == "head":
                groups = read_first(client.get_control_group_state)
                head = next((g for g in groups.joint_state if g.group_name == "head"), None)
                if head is None or len(head.joint_position) != 2:
                    raise RuntimeError("Expected two head joints")
                start_position = list(head.joint_position)
                if not all(math.isfinite(v) and abs(v) <= 0.2 for v in start_position):
                    raise RuntimeError("Prepare the head near its neutral posture first")
            previous_stamp = state.header.received_time
            last_update = time.monotonic()
            begin = time.monotonic()
            duration = 2.0 if args.action == "walk" else 4.0
            while True:
                elapsed = time.monotonic() - begin
                result, state = client.get_aurora_state()
                require(result)
                if state.header.received_time != previous_stamp:
                    previous_stamp = state.header.received_time
                    last_update = time.monotonic()
                if time.monotonic() - last_update > 1.0:
                    raise RuntimeError("Aurora state stopped updating")
                if state.current_state.id != initial_state or not client.get_lease().usable():
                    raise RuntimeError("State changed or control lease was lost")
                if args.action == "walk":
                    if elapsed >= duration:
                        break
                    require(client.set_velocity(0.1, 0.0, 0.0, 0.2))
                    motion_sent = True
                else:
                    # Smoothly move yaw out and back; preserve the measured pitch.
                    phase = min(elapsed / duration, 1.0)
                    position = list(start_position)
                    position[0] += 0.03 * (1.0 - math.cos(2.0 * math.pi * phase)) / 2.0
                    joint = JointCommand()
                    joint.group_name = "head"
                    joint.control_type = JointControlType.JOINT
                    joint.motor_mode = MotorMode.PD
                    joint.position = position
                    joint.velocity = [0.0, 0.0]
                    joint.effort = [0.0, 0.0]
                    command = ControlGroupCommand()
                    command.joint_cmd = [joint]
                    require(client.publish_control_group_command(command))
                    if phase >= 1.0:
                        break
                time.sleep(0.02)
    except KeyboardInterrupt:
        exit_code = 130
    except Exception as error:
        print(error, file=sys.stderr)
        exit_code = 1
    finally:
        # Cleanup must still reach stop() if a publish or release fails.
        try:
            if motion_sent and client.get_lease().usable():
                require(client.set_velocity(0.0, 0.0, 0.0, 0.2))
        except Exception as error:
            print(f"Zero velocity failed: {error}", file=sys.stderr)
            exit_code = exit_code or 1
        try:
            if acquired:
                require(client.release_lease().operation)
        except Exception as error:
            print(f"Lease release failed: {error}", file=sys.stderr)
            exit_code = exit_code or 1
        try:
            client.stop()
        except Exception as error:
            print(f"Client stop failed: {error}", file=sys.stderr)
            exit_code = exit_code or 1
    return exit_code


if __name__ == "__main__":
    raise SystemExit(main())
