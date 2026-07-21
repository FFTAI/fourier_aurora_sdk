#!/usr/bin/env python3
"""Request the GR3 Walk state and walk forward for three seconds."""

import time

from fourier_aurora_client import AuroraClient, Endpoint

GR3_WALK_STATE = 3
FORWARD_VELOCITY_MPS = 0.3
WALK_DURATION_SECONDS = 3.0


def print_result(operation, result):
    print(f'{operation} failed: code={result.code} message="{result.message}"')


def main():
    print(
        "GR3 walk_forward will request Walk (FSM 3), command "
        f"{FORWARD_VELOCITY_MPS} m/s in the base-frame forward direction "
        f"for {WALK_DURATION_SECONDS} s."
    )
    try:
        if input("Confirm the area is clear. Type YES to continue: ") != "YES":
            print("Cancelled.")
            return 0
    except KeyboardInterrupt:
        return 130

    client = AuroraClient.get_instance()
    interrupted = False
    try:
        options = client.config()
        options.enabled_endpoints = [
            Endpoint.FSM_SERVICE,
            Endpoint.AURORA_STATE_SUBSCRIBER,
            Endpoint.MOTION_COMMAND_PUBLISHER,
        ]
        client.configure(options)
        client.start()

        if not client.wait_for_endpoints(5.0):
            print("GR3 walk endpoints were not ready within 5 s.")
            return 1

        result = client.request_fsm_state(GR3_WALK_STATE)
        if not result.success():
            print_result("request_fsm_state(Walk)", result)
            return 1
        if not client.wait_for_fsm_state(GR3_WALK_STATE, 10.0):
            print("GR3 did not enter Walk (FSM 3) within 10 s.")
            return 1

        result = client.set_velocity(
            FORWARD_VELOCITY_MPS,
            0.0,
            0.0,
            WALK_DURATION_SECONDS,
        )
        if not result.success():
            print_result("set_velocity", result)
            return 1

        deadline = time.monotonic() + WALK_DURATION_SECONDS
        while time.monotonic() < deadline:
            time.sleep(min(0.05, deadline - time.monotonic()))
        return 0
    except KeyboardInterrupt:
        interrupted = True
        return 130
    except Exception as error:  # noqa: BLE001
        print(f"GR3 walk_forward failed: {error}")
        return 1
    finally:
        try:
            client.stop()
        except Exception as error:  # noqa: BLE001
            print(f"stop failed: {error}")
            if not interrupted:
                raise


if __name__ == "__main__":
    raise SystemExit(main())
