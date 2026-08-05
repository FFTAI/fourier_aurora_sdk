import importlib
import os
import subprocess
import sys
import textwrap

import fourier_aurora_client as sdk

native = importlib.import_module(
    "fourier_aurora_client._fourier_aurora_sdk"
)


def test_mock_lifecycle_getter_and_restart():
    client = sdk.AuroraClient.get_instance()
    options = client.config()
    options.source_id = 42
    options.enabled_endpoints = [
        sdk.Endpoint.MANAGE_LEASE_SERVICE,
        sdk.Endpoint.MOTION_STATE_SUBSCRIBER,
        sdk.Endpoint.MOTION_COMMAND_PUBLISHER,
    ]
    client.configure(options)

    client.start()
    assert client.state() == sdk.ClientState.RUNNING
    assert client.wait_for_endpoints(0.0)
    lease = client.register_lease([sdk.AuroraResource.MOTION_COMMAND])
    assert lease.usable()

    result, state = client.get_motion_state()
    assert result.code == sdk.STATE_NOT_RECEIVED
    assert state.stable_level == 0

    native._testing_inject_motion_state(42)
    result, state = client.get_motion_state()
    assert result.success()
    assert state.stable_level == 42

    command = sdk.MotionCommand()
    command.header.frame_id = "caller"
    command.lin_velocity = [0.1, 0.0, 0.0]
    result = client.publish_motion_command(command)
    assert result.success()
    result, transmitted = client.get_last_motion_command()
    assert result.success()
    assert transmitted.lin_velocity == command.lin_velocity
    assert transmitted.header.frame_id == ""
    assert command.header.frame_id == "caller"

    native._testing_set_match_all(False)
    result, state = client.get_motion_state()
    assert result.code == sdk.ENDPOINT_UNMATCHED
    assert state.stable_level == 0

    client.stop()
    client.stop()
    assert client.state() == sdk.ClientState.STOPPED
    client.start()
    assert client.state() == sdk.ClientState.RUNNING
    client.stop()


def test_exact_fsm_result_passthrough():
    client = sdk.AuroraClient.get_instance()
    options = client.config()
    options.enabled_endpoints = [sdk.Endpoint.FSM_SERVICE]
    client.configure(options)
    client.start()

    native._testing_set_fsm_result(1, "legacy failure")
    result = client.request_fsm_state(13)
    assert result.code == 1
    assert result.message == "legacy failure"

    native._testing_set_fsm_result(250, "")
    result = client.request_fsm_state(13)
    assert result.code == 250
    assert result.message == ""
    client.stop()


def test_blocking_bindings_release_gil_in_bounded_subprocess():
    script = textwrap.dedent(
        """
        import importlib
        import threading
        import time

        from fourier_aurora_client import AuroraClient, Endpoint

        native = importlib.import_module(
            "fourier_aurora_client._fourier_aurora_sdk"
        )
        native._testing_reset()
        client = AuroraClient.get_instance()
        initial_options = client.config()
        initial_options.source_id = 42
        client.configure(initial_options)

        native._testing_block_create()
        starter = threading.Thread(target=client.start)
        starter.start()
        assert native._testing_wait_create(1, 1.0)
        native._testing_release_create()
        starter.join(1.0)
        assert not starter.is_alive()

        native._testing_block_stop()
        stopper = threading.Thread(target=client.stop)
        stopper.start()
        assert native._testing_wait_stop(1, 1.0)
        native._testing_release_stop()
        stopper.join(1.0)
        assert not stopper.is_alive()

        options = client.config()
        options.enabled_endpoints = [
            Endpoint.FSM_SERVICE,
            Endpoint.AURORA_STATE_SUBSCRIBER,
        ]
        client.configure(options)
        client.start()

        native._testing_block_request()
        requester = threading.Thread(
            target=lambda: client.request_fsm_state(13)
        )
        requester.start()
        assert native._testing_wait_request(1, 1.0)
        native._testing_release_request()
        requester.join(1.0)
        assert not requester.is_alive()

        native._testing_set_match_all(False)
        endpoint_result = []
        endpoint_waiter = threading.Thread(
            target=lambda: endpoint_result.append(
                client.wait_for_endpoints(5.0)
            )
        )
        endpoint_waiter.start()
        time.sleep(0.05)
        client.stop()
        endpoint_waiter.join(1.0)
        assert not endpoint_waiter.is_alive()
        assert endpoint_result == [False]

        client.start()
        native._testing_set_match_all(False)
        fsm_result = []
        fsm_waiter = threading.Thread(
            target=lambda: fsm_result.append(
                client.wait_for_fsm_state(13, 5.0)
            )
        )
        fsm_waiter.start()
        time.sleep(0.05)
        client.stop()
        fsm_waiter.join(1.0)
        assert not fsm_waiter.is_alive()
        assert fsm_result == [False]
        """
    )
    environment = os.environ.copy()
    result = subprocess.run(
        [sys.executable, "-c", script],
        capture_output=True,
        text=True,
        timeout=8,
        env=environment,
        check=False,
    )
    assert result.returncode == 0, result.stdout + result.stderr
