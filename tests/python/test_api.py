import concurrent.futures
import math

import pytest

import fourier_aurora_client as sdk


@pytest.mark.parametrize("data_type", [sdk.EstimatorData, sdk.BaseData])
def test_estimator_root_link_is_distinct_from_reference_frame(data_type):
    data = data_type()
    assert data.root_link == ""
    data.header.frame_id = "world"
    data.root_link = "torso_link"
    assert data.root_link == "torso_link"
    assert data.header.frame_id == "world"


def test_singleton_wrapper_identity_and_defaults():
    client = sdk.AuroraClient.get_instance()
    assert client is sdk.AuroraClient.get_instance()
    assert client.state() == sdk.ClientState.STOPPED
    assert client.domain_id() == 123
    assert client.config().domain_id == 123
    assert client.config().topic_prefix == "aurora/"
    # The test fixture installs a valid Source Name so lifecycle tests can
    # start the default protected endpoints.
    assert client.config().source_name == "test-client"
    assert len(client.config().enabled_endpoints) == 12

    with concurrent.futures.ThreadPoolExecutor(max_workers=8) as executor:
        wrappers = list(
            executor.map(
                lambda _: sdk.AuroraClient.get_instance(),
                range(32),
            )
        )
    assert all(wrapper is client for wrapper in wrappers)


def test_removed_and_private_api_paths_are_absent():
    client = sdk.AuroraClient.get_instance()
    with pytest.raises(TypeError):
        sdk.AuroraClient(None)
    with pytest.raises(TypeError):
        sdk.AuroraClient.get_instance(123)

    for name in (
        "close",
        "close_instance",
        "set_fsm_state",
        "getMotionState",
        "requestFsmState",
        "on_motion_state",
        "onMotionState",
    ):
        assert not hasattr(client, name)


def test_exception_hierarchy():
    assert issubclass(sdk.AuroraSdkError, RuntimeError)
    assert issubclass(sdk.ConnectionError, sdk.AuroraSdkError)
    assert issubclass(sdk.InvalidArgumentError, sdk.AuroraSdkError)
    assert issubclass(sdk.InvalidStateError, sdk.AuroraSdkError)


def test_configuration_round_trip_and_copy_isolation():
    client = sdk.AuroraClient.get_instance()
    options = client.config()
    options.domain_id = 314
    options.namespace_name = "python_test"
    options.topic_prefix = "python_test/"
    options.dds_mode = 3
    options.is_ros_compatible = False
    options.use_namespace = False
    options.use_discovery_server = False
    options.service_timeout_ms = 250
    options.enabled_endpoints = [
        sdk.Endpoint.ESTIMATOR_DATA_SUBSCRIBER,
        sdk.Endpoint.FSM_SERVICE,
        sdk.Endpoint.ESTIMATOR_DATA_SUBSCRIBER,
    ]
    client.configure(options)

    canonical = client.config()
    assert canonical.domain_id == 314
    assert canonical.namespace_name == "python_test"
    assert canonical.topic_prefix == "python_test/"
    assert canonical.dds_mode == 3
    assert canonical.is_ros_compatible is False
    assert canonical.use_namespace is False
    assert canonical.use_discovery_server is False
    assert canonical.service_timeout_ms == 250
    assert canonical.enabled_endpoints == [
        sdk.Endpoint.FSM_SERVICE,
        sdk.Endpoint.ESTIMATOR_DATA_SUBSCRIBER,
    ]

    canonical.service_timeout_ms = 999
    assert client.config().service_timeout_ms == 250


@pytest.mark.parametrize("value", [True, False, -1, 2**32])
def test_domain_id_rejects_non_uint32(value):
    client = sdk.AuroraClient.get_instance()
    with pytest.raises(sdk.InvalidArgumentError):
        client.set_domain_id(value)
    assert client.domain_id() == 123


@pytest.mark.parametrize("value", [True, False, -1, 2**32])
def test_connection_options_domain_id_rejects_non_uint32(value):
    options = sdk.ConnectionOptions()
    with pytest.raises(sdk.InvalidArgumentError):
        options.domain_id = value


@pytest.mark.parametrize("value", [True, False, -1, 2**32])
def test_service_timeout_rejects_non_uint32(value):
    options = sdk.ConnectionOptions()
    with pytest.raises(sdk.InvalidArgumentError):
        options.service_timeout_ms = value


@pytest.mark.parametrize("value", [True, False, -1, 4, 2**32])
def test_dds_mode_validation(value):
    options = sdk.ConnectionOptions()
    with pytest.raises(sdk.InvalidArgumentError):
        options.dds_mode = value


@pytest.mark.parametrize(
    "value",
    [True, False, -1.0, math.inf, -math.inf, math.nan, "1"],
)
def test_wait_timeout_validation(value):
    client = sdk.AuroraClient.get_instance()
    with pytest.raises(sdk.InvalidArgumentError):
        client.wait_for_endpoints(value)


def test_positive_submillisecond_wait_is_accepted():
    client = sdk.AuroraClient.get_instance()
    assert client.wait_for_endpoints(0.0001) is False


@pytest.mark.parametrize("value", [True, False, -1, 2**32])
def test_fsm_id_rejects_non_uint32(value):
    client = sdk.AuroraClient.get_instance()
    with pytest.raises(sdk.InvalidArgumentError):
        client.request_fsm_state(value)
    with pytest.raises(sdk.InvalidArgumentError):
        client.wait_for_fsm_state(value, 0.0)


def test_tuple_getters_return_typed_defaults_on_failure():
    client = sdk.AuroraClient.get_instance()
    result, motion = client.get_motion_state()
    assert result.code == sdk.CLIENT_NOT_RUNNING
    assert isinstance(motion, sdk.MotionState)
    assert motion.stable_level == 0

    result, joint_values = client.get_joint_state(
        "left_leg", sdk.JointField.POSITION
    )
    assert result.code == sdk.CLIENT_NOT_RUNNING
    assert joint_values == []

    assert sdk.result_code_name(sdk.SUCCESS) == "SUCCESS"
    assert sdk.default_result_message(999) == "unknown result code"
