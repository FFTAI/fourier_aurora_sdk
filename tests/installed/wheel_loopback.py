import time

from fourier_aurora_client import (
    AuroraClient,
    AuroraResource,
    ClientState,
    Endpoint,
)


def main():
    client = AuroraClient.get_instance()
    client.stop()
    assert client.state() == ClientState.STOPPED
    assert client.domain_id() == 123
    assert client.config().domain_id == 123
    assert client.config().source_id == 0

    client.set_domain_id(214)
    options = client.config()
    assert options.domain_id == 214
    options.source_id = 77
    options.source_name = "wheel_loopback"
    options.namespace_name = "aurora_sdk_wheel_loopback"
    options.dds_mode = 0
    options.is_ros_compatible = False
    options.use_namespace = True
    options.use_discovery_server = False
    options.service_timeout_ms = 1000
    options.enabled_endpoints = [
        Endpoint.MANAGE_LEASE_SERVICE,
        Endpoint.AURORA_STATE_SUBSCRIBER,
        Endpoint.MOTION_COMMAND_PUBLISHER,
        Endpoint.FSM_SERVICE,
    ]
    client.configure(options)

    try:
        client.start()
        assert client.state() == ClientState.RUNNING
        assert client.wait_for_endpoints(5.0)
        lease = client.register_lease([AuroraResource.MOTION_COMMAND])
        assert lease.usable()

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
