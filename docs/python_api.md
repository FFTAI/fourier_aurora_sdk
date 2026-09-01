# Python API Reference

[中文](python_api_CN.md) · [Quick start](../README.md) ·
[Overview](overview.md) · [C++ API](cpp_api.md)

This page documents the user-facing `fourier_aurora_client` API. It begins
with the exported types and functions, then specifies every client method and
writable attribute used by applications.

```python
import fourier_aurora_client as aurora
client = aurora.AuroraClient.get_instance()
```

Do not construct `AuroraClient` directly. Linux x86_64 wheels are published
separately for standard-GIL CPython 3.10 through 3.13.

## API index

### Types

| Category | Exported types |
|---|---|
| Client and lifecycle | [`AuroraClient`](#auroraclient), [`ConnectionOptions`](#connectionoptions), [`ClientState`](#clientstate), [`Endpoint`](#endpoint), [`EndpointStatus`](#endpointstatus), [`EndpointMatchStatus`](#endpointmatchstatus) |
| Results and errors | [`OperationResult`](#operationresult), [`AuroraSdkError`](#exceptions), [`ConnectionError`](#exceptions), [`InvalidArgumentError`](#exceptions), [`InvalidStateError`](#exceptions) |
| Common and robot state | [`MessageHeader`](#messageheader), [`RobotInfo`](#robotinfo), [`StateInfo`](#stateinfo), [`AuroraState`](#aurorastate) |
| Motion | [`MotionState`](#motionstate), [`MotionCommand`](#motioncommand) |
| Control groups | [`JointControlType`](#jointcontroltype), [`MotorMode`](#motormode), [`JointField`](#jointfield), [`ControlGroupStateEntry`](#controlgroupstateentry), [`CartesianStateEntry`](#cartesianstateentry), [`CartesianCommand`](#cartesiancommand), [`ControlGroupState`](#controlgroupstate), [`JointCommand`](#jointcommand), [`ControlGroupCommand`](#controlgroupcommand) |
| Estimator | [`BaseData`](#basedata), [`ContactData`](#contactdata), [`EstimatorData`](#estimatordata) |
| Sub-controller | [`GroupOverrideParam`](#groupoverrideparam), [`ControllerParam`](#controllerparam), [`SubControllerCommand`](#subcontrollercommand) |
| Errors | [`ErrorCodeEntry`](#errorcodeentry), [`ErrorCodes`](#errorcodes) |
| Leases | [`AuroraResource`](#auroraresource), [`LeasePriority`](#leasepriority), [`LeaseState`](#leasestate), [`LeaseLossReason`](#leaselossreason), [`LeaseOption`](#leaseoption), [`LeaseResult`](#leaseresult) |
| Motor configuration | [`MotorConfigurationMode`](#motorconfigurationmode), [`MotorConfigurationGroupCommand`](#motorconfigurationgroupcommand), [`MotorConfigurationCommand`](#motorconfigurationcommand), [`MotorConfigurationGroupState`](#motorconfigurationgroupstate), [`MotorConfigurationState`](#motorconfigurationstate) |

### Functions

| Category | Public functions |
|---|---|
| Client lifecycle | [`AuroraClient.get_instance`](#get_instance), [`config`](#config), [`configure`](#configure), [`set_domain_id`](#set_domain_id), [`domain_id`](#domain_id), [`state`](#state), [`start`](#start), [`stop`](#stop) |
| Endpoint readiness | [`endpoint_match_status`](#endpoint_match_status), [`is_fsm_service_matched`](#is_fsm_service_matched), [`wait_for_endpoints`](#wait_for_endpoints) |
| State access | [`get_aurora_state`](#get_aurora_state), [`get_motion_state`](#get_motion_state), [`get_control_group_state`](#get_control_group_state), [`get_motor_configuration_state`](#get_motor_configuration_state), [`get_estimator_data`](#get_estimator_data), [`get_error_codes`](#get_error_codes), [`get_base_data`](#get_base_data), [`get_contact_data`](#get_contact_data), [`get_stable_level`](#get_stable_level), [`get_stand_pose`](#get_stand_pose), [`get_joint_state`](#get_joint_state), [`get_joint_command`](#get_joint_command), [`get_cartesian_state`](#get_cartesian_state), [`get_cartesian_command`](#get_cartesian_command), [`wait_for_fsm_state`](#wait_for_fsm_state) |
| Local command caches | [`get_last_motion_command`](#get_last_motion_command), [`get_last_control_group_command`](#get_last_control_group_command), [`get_last_motor_configuration_command`](#get_last_motor_configuration_command), [`get_last_sub_controller_command`](#get_last_sub_controller_command) |
| Commands and services | [`publish_motion_command`](#publish_motion_command), [`publish_control_group_command`](#publish_control_group_command), [`publish_motor_configuration_command`](#publish_motor_configuration_command), [`set_sub_controller`](#set_sub_controller), [`request_fsm_state`](#request_fsm_state), [`set_velocity`](#set_velocity), [`set_stand_pose`](#set_stand_pose), [`reset_unicontroller`](#reset_unicontroller) |
| Lease management | [`register_lease`](#register_lease), [`replace_lease`](#replace_lease), [`release_lease`](#release_lease), [`get_lease`](#get_lease), [`on_lease_changed`](#on_lease_changed) |
| Result helpers | [`result_code_name`, `default_result_message`, `OperationResult.success`, `LeaseResult.success`, `LeaseResult.usable`](#result-helpers) |

## Conventions

- Configure the process singleton only while `ClientState.STOPPED`.
- Use `try/finally` and call `stop()` explicitly.
- Wait timeouts are finite, nonnegative numbers in seconds.
- State/cache getters return `(OperationResult, value)`.
- Commands and services return `OperationResult`; lease calls return
  `LeaseResult`.
- Returned operation failures are not exceptions. Check them explicitly.
- `datetime.datetime` represents C++ time points; `datetime.timedelta`
  represents C++ durations.
- Numeric robot values use SI units unless stated otherwise.
- Fixed Cartesian pose arrays are `[x, y, z, qx, qy, qz, qw]`.

## Client and lifecycle types

### `AuroraClient`

Process-wide Python wrapper around the native singleton. Use
`AuroraClient.get_instance()`; direct construction raises `TypeError`. See
[Client functions](#client-functions) for all methods.

### `ConnectionOptions`

Construct with `aurora.ConnectionOptions()`. Every attribute is writable while
the object is local; `configure()` copies and validates it.

| Attribute | Python type | Default | Description |
|---|---|---:|---|
| `domain_id` | `int` | `123` | DDS domain used when the client starts its next transport session. Must fit `uint32`. |
| `namespace_name` | `str | None` | `None` | Explicit DDS namespace. |
| `topic_prefix` | `str` | `"aurora/"` | Literal prefix prepended to every SDK-managed Aurora topic and service name. Use `""` for no prefix. |
| `dds_mode` | `int` | `0` | FourierDDS mode; only `0..3` is accepted. |
| `is_ros_compatible` | `bool | None` | `None` | Optional ROS-compatible naming override. |
| `use_namespace` | `bool` | `True` | Enables namespaced endpoints. |
| `use_discovery_server` | `bool | None` | `None` | Optional discovery-server override. |
| `service_timeout_ms` | `int` | `1000` | Default synchronous service timeout. |
| `source_name` | `str` | `""` (invalid until set) | Required human-readable client identity matching `[A-Za-z0-9._-]{1,64}`. |
| `allow_emergency_lease_priority` | `bool` | `False` | Allows emergency-priority requests; Aurora still authorizes them. |
| `enabled_endpoints` | `list[Endpoint]` | all endpoints | Exact endpoint set created by `start()`. |

`source_name` is required for every client configuration and is validated by
`start()` before transport creation. Protected publishers also require
`MANAGE_LEASE_SERVICE`.

### `ClientState`

| Value | Description |
|---|---|
| `STOPPED` | No session; configuration is mutable. |
| `STARTING` | Transport startup is running. |
| `RUNNING` | Local transport is active. |
| `STOPPING` | Work is draining and shutdown is running. |

### `Endpoint`

| Value | Endpoint |
|---|---|
| `FSM_SERVICE` | FSM request service client. |
| `MANAGE_LEASE_SERVICE` | Lease-management client. |
| `MOTION_COMMAND_PUBLISHER` | Motion command publisher. |
| `CONTROL_GROUP_COMMAND_PUBLISHER` | Joint command publisher. |
| `MOTOR_CONFIGURATION_COMMAND_PUBLISHER` | Temporary motor-config publisher. |
| `SUB_CONTROLLER_SERVICE` | Universal Controller service client. |
| `AURORA_STATE_SUBSCRIBER` | Aurora/FSM/authority state. |
| `MOTION_STATE_SUBSCRIBER` | Motion state. |
| `CONTROL_GROUP_STATE_SUBSCRIBER` | Joint and Cartesian state. |
| `MOTOR_CONFIGURATION_STATE_SUBSCRIBER` | Motor-config state. |
| `ESTIMATOR_DATA_SUBSCRIBER` | Base/contact estimator state. |
| `ERROR_CODES_SUBSCRIBER` | Error-code state. |

### `EndpointStatus`

| Attribute | Description |
|---|---|
| `enabled` | Whether this endpoint was selected before startup. |
| `matched` | Whether DDS currently reports a compatible peer. |

### `EndpointMatchStatus`

Every attribute is an `EndpointStatus`: `fsm_service`,
`manage_lease_service`, `motion_command_publisher`,
`control_group_command_publisher`, `motor_configuration_command_publisher`,
`sub_controller_service`, `aurora_state_subscriber`,
`motion_state_subscriber`, `control_group_state_subscriber`,
`motor_configuration_state_subscriber`, `estimator_data_subscriber`, and
`error_codes_subscriber`.

## Result and error types

### `OperationResult`

| Attribute/method | Description |
|---|---|
| `code` | Writable integer SDK or Aurora result code; defaults to `CLIENT_ERROR`. |
| `message` | Writable human-readable detail. |
| `success()` | True only when `code == SUCCESS`. |

### Exceptions

| Type | Base | Description |
|---|---|---|
| `AuroraSdkError` | `RuntimeError` | Base thrown SDK failure. |
| `ConnectionError` | `AuroraSdkError` | Fatal transport startup/teardown failure. |
| `InvalidArgumentError` | `AuroraSdkError` | Invalid type, value, range, or combination. |
| `InvalidStateError` | `AuroraSdkError` | Lifecycle misuse. |

## State and command types

All data types below have a zero/default constructor and writable attributes.

### `MessageHeader`

| Attribute | Type | Description |
|---|---|---|
| `send_time` | `datetime.datetime` | Sender timestamp. |
| `frame_id` | `str` | Coordinate frame or source label. |
| `received_time` | `datetime.datetime` | Local SDK receive timestamp. |

### `RobotInfo`

`robot_type` identifies the robot family, `hardware_type` the revision, and
`end_effector_type` the installed end effector. All are strings.

### `StateInfo`

| Attribute | Description |
|---|---|
| `id` | FSM state ID. |
| `version` | State contract/configuration version. |
| `name` | User-facing state name. |
| `description` | Short state description. |
| `active` | Whether this entry is active. |

### `AuroraState`

| Attribute | Description |
|---|---|
| `header` | `MessageHeader`. |
| `robot_info` | Connected `RobotInfo`. |
| `switch_status` | Aurora state-switch status integer. |
| `current_state` | Current `StateInfo`. |
| `available_states` | List of reported `StateInfo` objects. |
| `authority_resources` | Supported recognized resources and their advisory authority state. Missing resources are unsupported; unknown future resources are ignored. |

### `MotionState`

| Attribute | Description |
|---|---|
| `header` | Message timing metadata. |
| `position`, `position_cmd` | Three-element measured/commanded base position. |
| `lin_velocity`, `lin_velocity_cmd` | Three-element measured/commanded linear velocity. |
| `attitude`, `attitude_cmd` | Three-element measured/commanded roll, pitch, yaw. |
| `ang_velocity`, `ang_velocity_cmd` | Three-element measured/commanded angular velocity. |
| `delta_height`, `delta_height_cmd` | Measured/commanded height offset. |
| `progress` | Motion progress integer. |
| `stable_level` | Stability level integer. |

### `MotionCommand`

| Attribute | Description |
|---|---|
| `header` | Outgoing metadata. |
| `position` | Three-element requested base position. |
| `lin_velocity` | Three-element base-frame linear velocity. |
| `attitude` | Three-element requested roll, pitch, yaw. |
| `ang_velocity` | Three-element angular velocity. |
| `delta_height` | Requested stand-height offset. |
| `gait_type` | Robot-specific integer gait selector. |
| `skills` | Eight robot-specific skill bytes. |
| `duration` | Command validity in seconds; defaults to 1.0. |

### `JointControlType`

`JOINT` uses logical-joint indexing; `MOTOR` uses hardware-motor indexing.

### `MotorMode`

Values: `PD`, `POSITION`, and `EFFORT`.

### `JointField`

Values: `POSITION`, `VELOCITY`, and `EFFORT`.

### `ControlGroupStateEntry`

`group_name` identifies the control group. `joint_position`,
`joint_velocity`, and `joint_effort` are numeric lists for that group.

### `CartesianStateEntry`

| Attribute | Description |
|---|---|
| `link_name` | URDF link name. |
| `pose` | Seven-element position and XYZW quaternion. |
| `twist` | Six-element linear/angular velocity. |
| `wrench` | Six-element force/torque. |

### `CartesianCommand`

`link_name` is an opaque, non-empty lookup key. `pose`, `twist`, and `wrench`
are lists that default to empty and mean “not commanded” when empty. At least
one must be supplied; present lengths are 7, 6, and 6. Aurora stores the raw
values unchanged, so the caller and task own frame semantics.

### `ControlGroupState`

| Attribute | Description |
|---|---|
| `header` | Message timing metadata. |
| `joint_state` | Measured `ControlGroupStateEntry` list. |
| `joint_cmd` | Observed commanded entry list. |
| `cartesian_state` | Measured `CartesianStateEntry` list. |
| `cartesian_cmd` | Latest observed sparse `CartesianCommand` list. |

### `JointCommand`

| Attribute | Description |
|---|---|
| `group_name` | Target Aurora control group. |
| `control_type` | `JointControlType`. |
| `motor_mode` | `MotorMode`. |
| `position` | Position command list. |
| `velocity` | Velocity command list. |
| `effort` | Feed-forward effort list. |

### `ControlGroupCommand`

`header` is outgoing metadata; `joint_cmd` is a list of `JointCommand` values
and `cartesian_cmd` is a list of `CartesianCommand` values. Cartesian entries
require `AuroraResource.CARTESIAN_COMMAND`.

### `BaseData`

| Attribute | Description |
|---|---|
| `header` | Message timing metadata. |
| `quat_xyzw`, `quat_wxyz` | Four-element base orientation in both orderings. |
| `rpy` | Roll, pitch, yaw. |
| `omega_W`, `acc_W` | World-frame angular velocity and acceleration. |
| `omega_B`, `acc_B` | Base-frame angular velocity and acceleration. |
| `vel_W`, `pos_W` | World-frame base velocity and position. |
| `vel_B` | Base-frame velocity. |
| `stable_level` | Estimator stability level. |

### `ContactData`

`header` is message metadata; `contact_force` and `contact_prob` are
per-contact numeric lists.

### `EstimatorData`

Attributes: `header`, `quat_xyzw`, `quat_wxyz`, `rpy`, `omega_W`, `acc_W`,
`omega_B`, `acc_B`, `vel_W`, `pos_W`, `vel_B`, `contact_force`,
`contact_prob`, and `stable_level`. Their meanings match `BaseData` and
`ContactData` above.

### `GroupOverrideParam`

`group_name` selects a control group; `control_param` stores the
controller-specific numeric override vector.

### `ControllerParam`

`controller_name` selects a Universal Controller sub-controller, `rate` is its
global activation/blend rate, and `group_override_params` contains
`GroupOverrideParam` entries.

### `SubControllerCommand`

`header` is outgoing metadata, `reset` requests reset, and
`controller_params` contains settings applied when reset is false.

### `ErrorCodeEntry`

`high32` and `low32` are the two 32-bit halves of one Aurora error code.

### `ErrorCodes`

`header` is message metadata, `source` is the error-source integer, and
`error_codes` is a list of `ErrorCodeEntry` values.

## Lease types

### `AuroraResource`

Values: `LEFT_LEG`, `RIGHT_LEG`, `WAIST`, `HEAD`, `LEFT_MANIPULATOR`,
`RIGHT_MANIPULATOR`, `LEFT_END_EFFECTOR`, `RIGHT_END_EFFECTOR`,
`MOTION_COMMAND`, and `CARTESIAN_COMMAND`.

### `LeasePriority`

Values: `NORMAL` and `EMERGENCY`. Emergency priority requires client opt-in
and Aurora authority permission.

### `LeaseState`

| Value | Description |
|---|---|
| `UNREGISTERED` | No lease. |
| `REGISTERING` | Registration in progress. |
| `ACTIVE` | Lease is locally usable until its deadline. |
| `REPLACING` | Atomic resource replacement in progress. |
| `RELEASING` | Release/publication drain in progress. |
| `LOST` | Former lease cannot be trusted. |

### `LeaseLossReason`

Values: `NONE`, `DEADLINE_EXPIRED`, `AUTHORITY_REJECTED`, `OPERATION_TIMEOUT`,
`INVALID_AUTHORITY_RESPONSE`, `PUBLICATION_DRAIN_TIMEOUT`, and
`INTERNAL_ERROR`.

### `LeaseOption`

| Attribute | Type | Default | Description |
|---|---|---:|---|
| `duration` | `datetime.timedelta` | 4 seconds | Requested lease period. |
| `priority` | `LeasePriority | None` | `None` | Optional explicit priority; none means normal. |
| `timeout` | `datetime.timedelta` | 500 ms | Public register/replace deadline. |

### `LeaseResult`

| Attribute/method | Description |
|---|---|
| `operation` | `OperationResult` for the call. |
| `state` | Current `LeaseState`. |
| `source_name` | Module identity. |
| `lease_id` | Authority-issued ID. |
| `resources` | Complete `AuroraResource` list. |
| `priority` | Lease priority. |
| `granted_duration` | Granted `datetime.timedelta`. |
| `remaining_duration` | Instantaneous remaining `datetime.timedelta`. |
| `loss_reason` | `LeaseLossReason`. |
| `success()` | Delegates to `operation.success()`. |
| `usable()` | Active with positive remaining duration. |

## Motor-configuration types

This surface is temporary and will move to the hardware-level SDK.

### `MotorConfigurationMode`

Values: `DISABLE`, `POSITION`, `VELOCITY`, `TORQUE`, and `PD`.

### `MotorConfigurationGroupCommand`

Attributes: `group_name`, `mode`, `position_kp`, `position_ki`,
`position_kd`, `pd_kp`, and `pd_kd`. Gain fields are numeric lists sized for
the target group.

### `MotorConfigurationCommand`

`header` is outgoing metadata; `groups` is a list of group commands.

### `MotorConfigurationGroupState`

Attributes: `group_name`, `mode`, `motor_error`, `position_kp`,
`position_ki`, `position_kd`, `pd_kp`, and `pd_kd`.

### `MotorConfigurationState`

`header` is message metadata; `groups` is a list of group states.

## Client functions

### Client lifecycle

#### `get_instance`

Declaration:

```python
AuroraClient.get_instance()
```

- Description: Gets the process-wide singleton without starting DDS.
- Parameters: None.
- Returns: The process-wide `AuroraClient`.

#### `config`

Declaration:

```python
client.config()
```

- Description: Reads the canonical connection configuration.
- Parameters: None.
- Returns: A copy of `ConnectionOptions`.

#### `configure`

Declaration:

```python
client.configure(options)
```

- Description: Replaces the complete connection configuration while stopped.
- Parameters: `options` — `ConnectionOptions` copied into the client.
- Returns: `None`; invalid options or lifecycle state raise an SDK exception.

#### `set_domain_id`

Declaration:

```python
client.set_domain_id(domain_id)
```

- Description: Convenience setter for `ConnectionOptions.domain_id`; updates the canonical stopped-stage configuration used by the next session.
- Parameters: `domain_id` — integer in the `uint32` range.
- Returns: `None`; the client must be stopped.

#### `domain_id`

Declaration:

```python
client.domain_id()
```

- Description: Convenience getter for the canonical `ConnectionOptions.domain_id` value.
- Parameters: None.
- Returns: Domain identifier as `int`.

#### `state`

Declaration:

```python
client.state()
```

- Description: Reads the local client lifecycle state.
- Parameters: None.
- Returns: The current `ClientState`.

#### `start`

Declaration:

```python
client.start()
```

- Description: Validates and freezes configuration, then starts enabled endpoints.
- Parameters: None.
- Returns: `None`; fatal startup failures raise an SDK exception.

#### `stop`

Declaration:

```python
client.stop()
```

- Description: Drains work, stops DDS, and clears session caches. Repeated calls are safe.
- Parameters: None.
- Returns: `None`; fatal shutdown failures raise an SDK exception.

### Endpoint readiness

#### `endpoint_match_status`

Declaration:

```python
client.endpoint_match_status()
```

- Description: Snapshots enabled and matched status for every endpoint.
- Parameters: None.
- Returns: `EndpointMatchStatus`.

#### `is_fsm_service_matched`

Declaration:

```python
client.is_fsm_service_matched()
```

- Description: Checks whether the enabled FSM service has a server match.
- Parameters: None.
- Returns: `True` when matched.

#### `wait_for_endpoints`

Declaration:

```python
client.wait_for_endpoints(timeout)
```

- Description: Waits until every enabled endpoint is matched, then waits another 0.2 seconds so volatile publishers can deliver their first sample. Match state is checked again before success. While discovery is pending, the SDK prints a match summary once per second; it prints an all-matched summary on success and an error with matched and unmatched endpoint names on timeout.
- Parameters: `timeout` — finite, nonnegative discovery timeout in seconds; the 0.2-second post-match delay is additional.
- Returns: `True` when all endpoints match, otherwise `False` on timeout.

### State access

Except for the wait, each function returns `(OperationResult, value)`.

Ignore the returned value when its `OperationResult` is not successful.

#### `get_aurora_state`

Declaration:

```python
client.get_aurora_state()
```

- Description: Reads the latest complete Aurora/FSM/authority snapshot.
- Parameters: None.
- Returns: `(OperationResult, AuroraState)`.

#### `get_motion_state`

Declaration:

```python
client.get_motion_state()
```

- Description: Reads the latest complete motion-state snapshot.
- Parameters: None.
- Returns: `(OperationResult, MotionState)`.

#### `get_control_group_state`

Declaration:

```python
client.get_control_group_state()
```

- Description: Reads the latest complete joint and Cartesian snapshot.
- Parameters: None.
- Returns: `(OperationResult, ControlGroupState)`.

#### `get_motor_configuration_state`

Declaration:

```python
client.get_motor_configuration_state()
```

- Description: Reads the latest temporary motor-configuration state.
- Parameters: None.
- Returns: `(OperationResult, MotorConfigurationState)`.

#### `get_estimator_data`

Declaration:

```python
client.get_estimator_data()
```

- Description: Reads the latest complete estimator snapshot.
- Parameters: None.
- Returns: `(OperationResult, EstimatorData)`.

#### `get_error_codes`

Declaration:

```python
client.get_error_codes()
```

- Description: Reads the latest asynchronous error-code snapshot.
- Parameters: None.
- Returns: `(OperationResult, ErrorCodes)`.

#### `get_base_data`

Declaration:

```python
client.get_base_data()
```

- Description: Projects base data from one estimator snapshot.
- Parameters: None.
- Returns: `(OperationResult, BaseData)`.

#### `get_contact_data`

Declaration:

```python
client.get_contact_data()
```

- Description: Projects contact data from one estimator snapshot.
- Parameters: None.
- Returns: `(OperationResult, ContactData)`.

#### `get_stable_level`

Declaration:

```python
client.get_stable_level()
```

- Description: Projects stability level from one motion snapshot.
- Parameters: None.
- Returns: `(OperationResult, int)`.

#### `get_stand_pose`

Declaration:

```python
client.get_stand_pose()
```

- Description: Projects `[height, pitch, yaw, stable_level]` from one motion snapshot.
- Parameters: None.
- Returns: `(OperationResult, list[float])` with four values.

#### `get_joint_state`

Declaration:

```python
client.get_joint_state(group_name, field)
```

- Description: Selects one measured joint field from a named control group.
- Parameters: `group_name` — control-group string; `field` — `JointField` selector.
- Returns: `(OperationResult, list[float])`.

#### `get_joint_command`

Declaration:

```python
client.get_joint_command(group_name, field)
```

- Description: Selects one commanded joint field observed by Aurora.
- Parameters: `group_name` — control-group string; `field` — `JointField` selector.
- Returns: `(OperationResult, list[float])`.

#### `get_cartesian_state`

Declaration:

```python
client.get_cartesian_state(link_name)
```

- Description: Selects measured Cartesian state by URDF link name.
- Parameters: `link_name` — requested URDF link.
- Returns: `(OperationResult, CartesianStateEntry)`.

#### `get_cartesian_command`

Declaration:

```python
client.get_cartesian_command(link_name)
```

- Description: Selects the latest observed sparse Cartesian command by exact key.
- Parameters: `link_name` — opaque lookup key.
- Returns: `(OperationResult, CartesianCommand)`; an absent key returns failure.

#### `wait_for_fsm_state`

Declaration:

```python
client.wait_for_fsm_state(state_id, timeout)
```

- Description: Waits until cached Aurora state reports the requested FSM ID.
- Parameters: `state_id` — `uint32` FSM ID; `timeout` — finite, nonnegative seconds.
- Returns: `True` when observed, otherwise `False` on timeout.

### Local command caches

These values are local intent, not execution feedback, and are cleared by
`stop()`.

#### `get_last_motion_command`

Declaration:

```python
client.get_last_motion_command()
```

- Description: Reads the last motion command published by this client.
- Parameters: None.
- Returns: `(OperationResult, MotionCommand)`.

#### `get_last_control_group_command`

Declaration:

```python
client.get_last_control_group_command()
```

- Description: Reads the last control-group command published by this client.
- Parameters: None.
- Returns: `(OperationResult, ControlGroupCommand)`.

#### `get_last_motor_configuration_command`

Declaration:

```python
client.get_last_motor_configuration_command()
```

- Description: Reads the last motor-configuration command published by this client.
- Parameters: None.
- Returns: `(OperationResult, MotorConfigurationCommand)`.

#### `get_last_sub_controller_command`

Declaration:

```python
client.get_last_sub_controller_command()
```

- Description: Reads the last successfully sent sub-controller command.
- Parameters: None.
- Returns: `(OperationResult, SubControllerCommand)`.

### Commands and services

Publish success means DDS accepted the sample, not that the robot executed it.

#### `publish_motion_command`

Declaration:

```python
client.publish_motion_command(command)
```

- Description: Publishes one complete motion command using the active motion lease.
- Parameters: `command` — `MotionCommand` to publish.
- Returns: `OperationResult` for publication and lease checks.

#### `publish_control_group_command`

Declaration:

```python
client.publish_control_group_command(command)
```

- Description: Publishes joint and/or sparse Cartesian commands after checking all required resources.
- Parameters: `command` — `ControlGroupCommand` to publish.
- Returns: `OperationResult` for publication and lease checks.

#### `publish_motor_configuration_command`

Declaration:

```python
client.publish_motor_configuration_command(command)
```

- Description: Publishes temporary motor gains after checking targeted resources.
- Parameters: `command` — `MotorConfigurationCommand` to publish.
- Returns: `OperationResult` for publication and lease checks.

#### `set_sub_controller`

Declaration:

```python
client.set_sub_controller(command)
```

- Description: Requests a Universal Controller sub-controller update without a lease.
- Parameters: `command` — `SubControllerCommand` reset or parameter request.
- Returns: Aurora's `OperationResult`; fails without an active Universal Controller task.

#### `request_fsm_state`

Declaration:

```python
client.request_fsm_state(state_id)
```

- Description: Requests one Aurora FSM state without a lease.
- Parameters: `state_id` — integer in the `uint32` range.
- Returns: Aurora's FSM-service `OperationResult`.

#### `set_velocity`

Declaration:

```python
client.set_velocity(vx, vy, yaw, duration, gait_type=0)
```

- Description: Builds and publishes a base-frame velocity `MotionCommand`.
- Parameters: `vx`, `vy` — m/s; `yaw` — rad/s; `duration` — validity in seconds; `gait_type` — robot-specific gait selector.
- Returns: `OperationResult` for publication and lease checks.

#### `set_stand_pose`

Declaration:

```python
client.set_stand_pose(delta_height, delta_pitch, delta_yaw, duration=1.0)
```

- Description: Builds and publishes nominal-stand pose offsets.
- Parameters: `delta_height` — metres; `delta_pitch`, `delta_yaw` — radians; `duration` — validity in seconds.
- Returns: `OperationResult` for publication and lease checks.

#### `reset_unicontroller`

Declaration:

```python
client.reset_unicontroller()
```

- Description: Sends a sub-controller request with `reset=True`.
- Parameters: None.
- Returns: Aurora's sub-controller `OperationResult`.

### Lease management

Active leases renew automatically and are not persisted. Keep lease callbacks
short and do not call `stop()` from them.

#### `register_lease`

Declaration:

```python
client.register_lease(resources, option=None)
```

- Description: Registers the module's first complete leased resource set.
- Parameters: `resources` — unique `AuroraResource` values; `option` — optional `LeaseOption`.
- Returns: `LeaseResult` containing Aurora's decision and local lease state.

#### `replace_lease`

Declaration:

```python
client.replace_lease(resources, option=None)
```

- Description: Atomically requests a new complete resource set and lease ID. Definitive rejection retains a still-valid old lease.
- Parameters: `resources` — replacement resources; `option` — optional `LeaseOption`.
- Returns: `LeaseResult` for the replacement and resulting local state.

#### `release_lease`

Declaration:

```python
client.release_lease()
```

- Description: Releases the active module lease; repeated release is safe.
- Parameters: None.
- Returns: The resulting `LeaseResult`.

#### `get_lease`

Declaration:

```python
client.get_lease()
```

- Description: Reads the current module lease without contacting Aurora.
- Parameters: None.
- Returns: An instantaneous local `LeaseResult` snapshot.

#### `on_lease_changed`

Declaration:

```python
client.on_lease_changed(callback)
```

- Description: Sets the serialized lease-change callback while stopped.
- Parameters: `callback` — callable receiving `LeaseResult`, or `None` to clear it.
- Returns: `None`.

## Result helpers

| Function | Description |
|---|---|
| `result_code_name(code)` | Returns a stable symbolic name or `UNKNOWN_RESULT_CODE`. |
| `default_result_message(code)` | Returns the default English message. |
| `OperationResult.success()` | Tests `code == SUCCESS`. |
| `LeaseResult.success()` | Tests the embedded operation. |
| `LeaseResult.usable()` | Tests active state and remaining duration. |
