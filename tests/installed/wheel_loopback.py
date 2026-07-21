import time

from fourier_aurora_client import AuroraClient, ClientState, Endpoint


def main():
    client = AuroraClient.get_instance()
    client.stop()
    assert client.state() == ClientState.STOPPED
    assert client.domain_id() == 123
    assert client.user_id() == "aurora_client"

    client.set_domain_id(214)
    client.set_user_id("wheel_loopback")
    options = client.config()
    options.namespace_name = "aurora_sdk_wheel_loopback"
    options.dds_mode = 0
    options.is_ros_compatible = False
    options.use_namespace = True
    options.use_discovery_server = False
    options.service_timeout_ms = 1000
    options.enabled_endpoints = [
        Endpoint.AURORA_STATE_SUBSCRIBER,
        Endpoint.MOTION_COMMAND_PUBLISHER,
        Endpoint.FSM_SERVICE,
    ]
    client.configure(options)

    try:
        client.start()
        assert client.state() == ClientState.RUNNING
        assert client.wait_for_endpoints(5.0)

        deadline = time.monotonic() + 3.0
        while True:
            result, state = client.get_aurora_state()
            if result.success() and state.current_state.id == 13:
                break
            if time.monotonic() >= deadline:
                raise AssertionError(
                    f"state was not received: {result.code} {result.message}"
                )
            time.sleep(0.05)

        result = client.request_fsm_state(13)
        assert result.code == 213
        assert result.message == "state=13"

        result = client.set_velocity(0.1, 0.0, 0.0, 0.1)
        assert result.success()
    finally:
        client.stop()


if __name__ == "__main__":
    main()
