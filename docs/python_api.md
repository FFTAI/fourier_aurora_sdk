# Python API Reference

[中文](python_api_CN.md) · [Quick start](../README.md) ·
[Overview](overview.md) · [C++ API](cpp_api.md)

The Python package wraps the same native client as the C++ API and presents
snake-case method names:

```python
import fourier_aurora_client as aurora
client = aurora.AuroraClient.get_instance()
```

Do not construct `AuroraClient` directly. The package requires a standard-GIL
CPython build; the current stage-one wheel is built for CPython 3.10 x86_64.

## Calling conventions

- Configure the singleton only while `ClientState.STOPPED`.
- Always use `try/finally` and call `stop()`.
- Wait timeouts (`wait_for_endpoints`, `wait_for_fsm_state`) are finite,
  nonnegative numbers in seconds.
- State getters return `(OperationResult, value)`.
- Commands and services return `OperationResult`; lease functions return
  `LeaseResult`.
- Check results explicitly. Publish success is not robot execution
  acknowledgement.

## Configuration and lifecycle functions

| Function | Return | Purpose |
|---|---|---|
| `AuroraClient.get_instance()` | `AuroraClient` | Return the process singleton without starting DDS |
| `config()` | `ConnectionOptions` | Return a copy of current configuration |
| `configure(options)` | `None` | Replace the complete stopped-stage configuration |
| `set_domain_id(domain_id)` | `None` | Set the next session's DDS domain; default is `123` |
| `domain_id()` | `int` | Read configured DDS domain |
| `state()` | `ClientState` | Read local lifecycle state |
| `start()` | `None` | Freeze configuration and start transport |
| `stop()` | `None` | Drain work and synchronously stop transport |
| `endpoint_match_status()` | `EndpointMatchStatus` | Return all enabled/matched flags |
| `is_fsm_service_matched()` | `bool` | Check the enabled FSM service match |
| `wait_for_endpoints(timeout)` | `bool` | Wait for every enabled endpoint to match |
| `wait_for_fsm_state(state_id, timeout)` | `bool` | Wait for the cached Aurora FSM ID |

`ConnectionOptions` fields are `namespace_name`, `dds_mode`,
`is_ros_compatible`, `use_namespace`, `use_discovery_server`,
`service_timeout_ms`, `source_id`, `source_name`,
`allow_emergency_lease_priority`, and `enabled_endpoints`. See the
[C++ configuration table](cpp_api.md#connectionoptions) for defaults and
semantics; the Python fields have the same values.

```python
options = client.config()
options.namespace_name = "robot_1"
options.source_id = 42
options.enabled_endpoints = [
    aurora.Endpoint.MANAGE_LEASE_SERVICE,
    aurora.Endpoint.MOTION_COMMAND_PUBLISHER,
]
client.configure(options)
```

Python endpoint values use upper snake case:

```text
FSM_SERVICE                    MANAGE_LEASE_SERVICE
MOTION_COMMAND_PUBLISHER       CONTROL_GROUP_COMMAND_PUBLISHER
MOTOR_CONFIGURATION_COMMAND_PUBLISHER
SUB_CONTROLLER_SERVICE
AURORA_STATE_SUBSCRIBER        MOTION_STATE_SUBSCRIBER
CONTROL_GROUP_STATE_SUBSCRIBER MOTOR_CONFIGURATION_STATE_SUBSCRIBER
ESTIMATOR_DATA_SUBSCRIBER      ERROR_CODES_SUBSCRIBER
```

## State getter functions

Every function in this section returns `(result, value)`. When the result is
not successful, ignore the default-constructed value.

| Function | Value | Required endpoint |
|---|---|---|
| `get_aurora_state()` | `AuroraState` | `AURORA_STATE_SUBSCRIBER` |
| `get_motion_state()` | `MotionState` | `MOTION_STATE_SUBSCRIBER` |
| `get_control_group_state()` | `ControlGroupState` | `CONTROL_GROUP_STATE_SUBSCRIBER` |
| `get_motor_configuration_state()` | `MotorConfigurationState` | `MOTOR_CONFIGURATION_STATE_SUBSCRIBER` |
| `get_estimator_data()` | `EstimatorData` | `ESTIMATOR_DATA_SUBSCRIBER` |
| `get_error_codes()` | `ErrorCodes` | `ERROR_CODES_SUBSCRIBER` |
| `get_base_data()` | `BaseData` projected from estimator | `ESTIMATOR_DATA_SUBSCRIBER` |
| `get_contact_data()` | `ContactData` projected from estimator | `ESTIMATOR_DATA_SUBSCRIBER` |
| `get_stable_level()` | `int` from motion state | `MOTION_STATE_SUBSCRIBER` |
| `get_stand_pose()` | `[height, pitch, yaw, stable_level]` | `MOTION_STATE_SUBSCRIBER` |
| `get_joint_state(group_name, field)` | Selected measured joint list | `CONTROL_GROUP_STATE_SUBSCRIBER` |
| `get_joint_command(group_name, field)` | Selected observed command list | `CONTROL_GROUP_STATE_SUBSCRIBER` |
| `get_cartesian_state(link_name)` | `CartesianStateEntry` | `CONTROL_GROUP_STATE_SUBSCRIBER` |
| `get_cartesian_command(link_name)` | `CartesianStateEntry` | `CONTROL_GROUP_STATE_SUBSCRIBER` |

`JointField` values are `POSITION`, `VELOCITY`, and `EFFORT`. Cartesian state
entries identify links by their URDF link name. Cartesian command is observed
state only; the SDK does not publish Cartesian commands in this release.

The following functions return this client's last local command cache:

| Function | Value |
|---|---|
| `get_last_motion_command()` | `MotionCommand` |
| `get_last_control_group_command()` | `ControlGroupCommand` |
| `get_last_motor_configuration_command()` | `MotorConfigurationCommand` |
| `get_last_sub_controller_command()` | `SubControllerCommand` |

These values are not robot execution feedback and are cleared at `stop()`.

## Command and service functions

| Function | Required endpoint / lease | Purpose |
|---|---|---|
| `publish_motion_command(command)` | Motion publisher; `MOTION_COMMAND` lease | Publish a complete `MotionCommand` |
| `publish_control_group_command(command)` | Control-group publisher; every targeted joint resource | Publish named joint-group commands |
| `publish_motor_configuration_command(command)` | Motor-config publisher; every targeted joint resource | Publish temporary customized motor gains |
| `set_sub_controller(command)` | Sub-controller service; no lease | Apply/reset Universal Controller sub-controller parameters |
| `request_fsm_state(state_id)` | FSM service; no lease | Request an Aurora FSM state |
| `set_velocity(vx, vy, yaw, duration, gait_type=0)` | Motion publisher and lease | Build a base-frame motion command |
| `set_stand_pose(height, pitch, yaw, duration=1.0)` | Motion publisher and lease | Build a stand-offset motion command |
| `reset_unicontroller()` | Sub-controller service; no lease | Send `SubControllerCommand(reset=True)` |

`set_sub_controller()` returns a failure when the active task does not contain
a Universal Controller. Motor configuration is temporary and will move to the
hardware-level SDK.

### Build a control-group command

```python
joint = aurora.JointCommand()
joint.group_name = "left_manipulator"
joint.control_type = aurora.JointControlType.JOINT
joint.motor_mode = aurora.MotorMode.PD
joint.position = [0.0, 0.2, 0.0, -0.7]
joint.velocity = [0.0] * len(joint.position)
joint.effort = [0.0] * len(joint.position)

command = aurora.ControlGroupCommand()
command.joint_cmd = [joint]
result = client.publish_control_group_command(command)
```

Aurora validates robot-specific group names, vector sizes, modes, active FSM,
and capabilities.

## Control lease functions

| Function | Return | Purpose |
|---|---|---|
| `register_lease(resources, option=None)` | `LeaseResult` | Register the first complete resource set |
| `replace_lease(resources, option=None)` | `LeaseResult` | Atomically replace the complete resource set |
| `release_lease()` | `LeaseResult` | Idempotently release the module lease |
| `get_lease()` | `LeaseResult` | Read an instantaneous local lease snapshot |
| `on_lease_changed(callback)` | `None` | Set a lease-change callback while stopped; pass `None` to clear |

Available `AuroraResource` values are `LEFT_LEG`, `RIGHT_LEG`, `WAIST`, `HEAD`,
`LEFT_MANIPULATOR`, `RIGHT_MANIPULATOR`, `LEFT_END_EFFECTOR`,
`RIGHT_END_EFFECTOR`, and `MOTION_COMMAND`.

`LeaseOption.duration` and `.timeout` are `datetime.timedelta` values because
they map to C++ chrono durations. Defaults are two seconds and 500 ms.

```python
from datetime import timedelta

option = aurora.LeaseOption()
option.duration = timedelta(seconds=2)
option.timeout = timedelta(milliseconds=500)
option.priority = aurora.LeasePriority.NORMAL

lease = client.register_lease(
    [aurora.AuroraResource.LEFT_MANIPULATOR], option
)
if not lease.usable():
    raise RuntimeError(
        f"lease failed [{lease.operation.code}] {lease.operation.message}"
    )
```

`LeaseResult` fields are `operation`, `state`, `source_id`, `source_name`,
`lease_id`, `resources`, `priority`, `granted_duration`, `remaining_duration`,
and `loss_reason`. `success()` describes the API operation; `usable()` means the
returned snapshot can authorize commands now. The SDK automatically renews an
active lease and stores no lease data on disk.

Only lease-change callback registration is exposed in Python in this release.
Keep the callback short, exception-safe, and free of calls to `stop()`.

## Exported types

All data classes use construct-and-assign semantics:

```python
command = aurora.MotionCommand()
command.lin_velocity = [0.1, 0.0, 0.0]
command.duration = 1.0
```

| Area | Exported types |
|---|---|
| Lifecycle | `ConnectionOptions`, `ClientState`, `Endpoint`, `EndpointStatus`, `EndpointMatchStatus` |
| Results | `OperationResult`; result constants and helper functions |
| Robot/FSM | `MessageHeader`, `RobotInfo`, `StateInfo`, `AuroraState` |
| Motion | `MotionCommand`, `MotionState` |
| Control group | `JointField`, `JointControlType`, `MotorMode`, `ControlGroupStateEntry`, `CartesianStateEntry`, `JointCommand`, `ControlGroupState`, `ControlGroupCommand` |
| Estimator | `BaseData`, `ContactData`, `EstimatorData` |
| Motor configuration | `MotorConfigurationMode`, group command/state and command/state container types |
| Sub-controller | `GroupOverrideParam`, `ControllerParam`, `SubControllerCommand` |
| Errors | `ErrorCodeEntry`, `ErrorCodes` |
| Lease | `AuroraResource`, `LeasePriority`, `LeaseState`, `LeaseLossReason`, `LeaseOption`, `LeaseResult` |

See the [C++ data-type summary](cpp_api.md#data-types) for field meanings; the
Python attributes have the same snake-case names already used by the C++
struct fields. Native `std::chrono::system_clock::time_point` fields appear as
Python `datetime.datetime`; duration fields appear as `datetime.timedelta`.

## Results, helpers, and exceptions

`OperationResult` has writable `code` and `message` fields and a `success()`
method. The package exports `SUCCESS` and the local client constants
`CLIENT_ERROR`, `CLIENT_NOT_RUNNING`, `ENDPOINT_DISABLED`,
`ENDPOINT_UNMATCHED`, `PUBLISH_FAILED`, `SERVICE_TIMEOUT`, `TRANSPORT_ERROR`,
`INVALID_AURORA_RESPONSE`, and `STATE_NOT_RECEIVED`.

Use `result_code_name(code)` and `default_result_message(code)` for all
SDK-known result numbers, including lease and sub-controller results. Aurora
service result codes and messages are returned unchanged.

The exported exception hierarchy mirrors C++:

```text
RuntimeError
└── AuroraSdkError
    ├── ConnectionError
    ├── InvalidArgumentError
    └── InvalidStateError
```

Catch returned operation failures where they happen, and reserve exceptions
for invalid API use or fatal transport lifecycle failures.
