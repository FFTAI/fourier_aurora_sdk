# C++ API Reference

[中文](cpp_api_CN.md) · [Quick start](../README.md) ·
[Overview](overview.md) · [Python API](python_api.md)

This page documents the user-facing C++ API for Fourier Aurora SDK. It starts
with a compact index, followed by the specification of each public client type
and function used by applications.

```cpp
#include "fourier_aurora_sdk/aurora_client.hpp"
namespace sdk = fourier::aurora_sdk;
```

All names are in `fourier::aurora_sdk`. The library requires C++17.

## API index

### Types

| Category | Public types |
|---|---|
| Client and lifecycle | [`AuroraClient`](#auroraclient), [`ConnectionOptions`](#connectionoptions), [`ClientState`](#clientstate), [`Endpoint`](#endpoint), [`EndpointStatus`](#endpointstatus), [`EndpointMatchStatus`](#endpointmatchstatus) |
| Results and errors | [`OperationResult`](#operationresult), [`AuroraSdkError`](#exceptions), [`ConnectionError`](#exceptions), [`InvalidArgumentError`](#exceptions), [`InvalidStateError`](#exceptions) |
| Common and robot state | [`MessageHeader`](#messageheader), [`RobotInfo`](#robotinfo), [`StateInfo`](#stateinfo), [`AuroraState`](#aurorastate) |
| Motion | [`MotionState`](#motionstate), [`MotionCommand`](#motioncommand) |
| Control groups | [`JointControlType`](#jointcontroltype), [`MotorMode`](#motormode), [`JointField`](#jointfield), [`ControlGroupStateEntry`](#controlgroupstateentry), [`CartesianStateEntry`](#cartesianstateentry), [`CartesianCommand`](#cartesiancommand), [`ControlGroupState`](#controlgroupstate), [`JointCommand`](#jointcommand), [`ControlGroupCommand`](#controlgroupcommand) |
| Estimator | [`BaseData`](#basedata), [`ContactData`](#contactdata), [`EstimatorData`](#estimatordata) |
| Sub-controller | [`GroupOverrideParam`](#groupoverrideparam), [`ControllerParam`](#controllerparam), [`SubControllerCommand`](#subcontrollercommand) |
| Errors | [`ErrorCodeEntry`](#errorcodeentry), [`ErrorCodes`](#errorcodes) |
| Leases | [`AuroraResource`](#auroraresource), [`LeasePriority`](#leasepriority), [`LeaseState`](#leasestate), [`LeaseLossReason`](#leaselossreason), [`LeaseOption`](#leaseoption), [`LeaseResult`](#leaseresult), [`AuthorityResourceStatus`](#authorityresourcestatus) |
| Motor configuration | [`MotorConfigurationMode`](#motorconfigurationmode), [`MotorConfigurationGroupCommand`](#motorconfigurationgroupcommand), [`MotorConfigurationCommand`](#motorconfigurationcommand), [`MotorConfigurationGroupState`](#motorconfigurationgroupstate), [`MotorConfigurationState`](#motorconfigurationstate) |
| Callback handlers | [`AuroraStateHandler`](#callback-handler-types), [`MotionStateHandler`](#callback-handler-types), [`ControlGroupStateHandler`](#callback-handler-types), [`MotorConfigurationStateHandler`](#callback-handler-types), [`EstimatorDataHandler`](#callback-handler-types), [`ErrorCodesHandler`](#callback-handler-types), [`LeaseChangedHandler`](#callback-handler-types) |

### Functions

| Category | Public functions |
|---|---|
| Client lifecycle | [`AuroraClient::getInstance`](#getinstance), [`config`](#config), [`configure`](#configure), [`setDomainId`](#setdomainid), [`domainId`](#domainid), [`state`](#state), [`start`](#start), [`stop`](#stop) |
| Endpoint readiness | [`endpointMatchStatus`](#endpointmatchstatus-1), [`isFsmServiceMatched`](#isfsmservicematched), [`waitForEndpoints`](#waitforendpoints) |
| State access | [`getAuroraState`](#getaurorastate), [`getMotionState`](#getmotionstate), [`getControlGroupState`](#getcontrolgroupstate), [`getMotorConfigurationState`](#getmotorconfigurationstate), [`getEstimatorData`](#getestimatordata), [`getErrorCodes`](#geterrorcodes), [`getBaseData`](#getbasedata), [`getContactData`](#getcontactdata), [`getStableLevel`](#getstablelevel), [`getStandPose`](#getstandpose), [`getJointState`](#getjointstate), [`getJointCommand`](#getjointcommand), [`getCartesianState`](#getcartesianstate), [`getCartesianCommand`](#getcartesiancommand), [`waitForFsmState`](#waitforfsmstate) |
| Local command caches | [`getLastMotionCommand`](#getlastmotioncommand), [`getLastControlGroupCommand`](#getlastcontrolgroupcommand), [`getLastMotorConfigurationCommand`](#getlastmotorconfigurationcommand), [`getLastSubControllerCommand`](#getlastsubcontrollercommand) |
| Commands and services | [`publishMotionCommand`](#publishmotioncommand), [`publishControlGroupCommand`](#publishcontrolgroupcommand), [`publishMotorConfigurationCommand`](#publishmotorconfigurationcommand), [`setSubController`](#setsubcontroller), [`requestFsmState`](#requestfsmstate), [`setVelocity`](#setvelocity), [`setStandPose`](#setstandpose), [`resetUnicontroller`](#resetunicontroller) |
| Lease management | [`registerLease`](#registerlease), [`replaceLease`](#replacelease), [`releaseLease`](#releaselease), [`getLease`](#getlease) |
| Callbacks | [`onAuroraState`](#onaurorastate), [`onMotionState`](#onmotionstate), [`onControlGroupState`](#oncontrolgroupstate), [`onMotorConfigurationState`](#onmotorconfigurationstate), [`onEstimatorData`](#onestimatordata), [`onErrorCodes`](#onerrorcodes), [`onLeaseChanged`](#onleasechanged) |
| Result helpers | [`resultCodeName`, `defaultResultMessage`, `OperationResult::success`, `LeaseResult::success`, `LeaseResult::usable`](#result-helpers) |

## Conventions

- `AuroraClient` is a process-wide singleton. Obtain it with `getInstance()`.
- Configuration and callback registration are allowed only while stopped.
- `start()` and `stop()` control local DDS transport; `Running` does not mean
  the robot is ready.
- Normal operation failures are returned as `OperationResult` or
  `LeaseResult`. Exceptions report invalid API use or fatal lifecycle failure.
- A failed C++ getter leaves its output argument unchanged.
- Timeouts and lease periods use `std::chrono::milliseconds`.
- Numeric robot values use SI units unless stated otherwise.
- Fixed Cartesian pose arrays are `[x, y, z, qx, qy, qz, qw]`; twist and
  wrench arrays are `[x, y, z, rx, ry, rz]`.

## Client and lifecycle types

### `AuroraClient`

Process-wide, restartable high-level client. It cannot be copied, moved, or
constructed directly. See [Client functions](#client-functions) for every
member function.

### `ConnectionOptions`

Complete configuration frozen by `AuroraClient::start()`.

| Member | Type | Default | Description |
|---|---|---:|---|
| `domain_id` | `uint32_t` | `123` | DDS domain used when the client starts its next transport session. |
| `namespace_name` | `std::optional<std::string>` | unset | Explicit DDS namespace. When unset, the transport may discover one. |
| `topic_prefix` | `std::string` | `"aurora/"` | Literal prefix prepended to every SDK-managed Aurora topic and service name. Set to `""` for no prefix. |
| `dds_mode` | `uint8_t` | `0` | FourierDDS mode in the range `0..3`. |
| `is_ros_compatible` | `std::optional<bool>` | unset | Overrides ROS-compatible DDS naming when set. |
| `use_namespace` | `bool` | `true` | Enables namespace prefixes on endpoints. |
| `use_discovery_server` | `std::optional<bool>` | unset | Overrides discovery-server use when set. |
| `service_timeout_ms` | `uint32_t` | `1000` | Default timeout for synchronous services. |
| `source_name` | `std::string` | `""` (invalid until set) | Required human-readable client identity matching `[A-Za-z0-9._-]{1,64}`. |
| `allow_emergency_lease_priority` | `bool` | `false` | Lets this client request emergency lease priority; Aurora still authorizes it. |
| `enabled_endpoints` | `std::vector<Endpoint>` | all endpoints | Exact endpoint set created by `start()`. |

`source_name` is required for every client configuration and is validated by
`start()` before transport creation. A protected publisher also requires
`Endpoint::ManageLeaseService`.

### `ClientState`

| Value | Description |
|---|---|
| `Stopped` | No transport session exists; configuration is mutable. |
| `Starting` | Transport startup is in progress. |
| `Running` | Local transport is active. |
| `Stopping` | Work is draining and transport shutdown is in progress. |

### `Endpoint`

| Value | Endpoint created when enabled |
|---|---|
| `FsmService` | FSM request client. |
| `ManageLeaseService` | Lease-management service client. |
| `MotionCommandPublisher` | Robot motion command publisher. |
| `ControlGroupCommandPublisher` | Joint control-group command publisher. |
| `MotorConfigurationCommandPublisher` | Temporary motor-configuration publisher. |
| `SubControllerService` | Universal Controller sub-controller service client. |
| `AuroraStateSubscriber` | Aurora/FSM/authority state subscriber. |
| `MotionStateSubscriber` | Robot motion state subscriber. |
| `ControlGroupStateSubscriber` | Joint and Cartesian state subscriber. |
| `MotorConfigurationStateSubscriber` | Motor-configuration state subscriber. |
| `EstimatorDataSubscriber` | Base and contact estimator subscriber. |
| `ErrorCodesSubscriber` | Error-code subscriber. |

### `EndpointStatus`

| Member | Type | Description |
|---|---|---|
| `enabled` | `bool` | Whether the endpoint was selected before startup. |
| `matched` | `bool` | Whether DDS currently reports a compatible peer. |

### `EndpointMatchStatus`

Each member is an `EndpointStatus` for the correspondingly named endpoint:
`fsm_service`, `manage_lease_service`, `motion_command_publisher`,
`control_group_command_publisher`, `motor_configuration_command_publisher`,
`sub_controller_service`, `aurora_state_subscriber`,
`motion_state_subscriber`, `control_group_state_subscriber`,
`motor_configuration_state_subscriber`, `estimator_data_subscriber`, and
`error_codes_subscriber`.

## Result and error types

### `OperationResult`

| Member | Type | Description |
|---|---|---|
| `code` | `uint32_t` | SDK or Aurora result code. Defaults to `CLIENT_ERROR`. |
| `message` | `std::string` | Human-readable result detail. |
| `success()` | `bool` | Returns true only when `code == SUCCESS`. |

### Exceptions

| Type | Base | Description |
|---|---|---|
| `AuroraSdkError` | `std::runtime_error` | Base class for thrown SDK failures. |
| `ConnectionError` | `AuroraSdkError` | Fatal transport startup or teardown failure. |
| `InvalidArgumentError` | `AuroraSdkError` | Invalid value, combination, or range. |
| `InvalidStateError` | `AuroraSdkError` | Operation is invalid in the current lifecycle state. |

Each exception has an explicit/inherited constructor taking a message string.

## State and command types

### `MessageHeader`

| Member | Type | Description |
|---|---|---|
| `send_time` | `std::chrono::system_clock::time_point` | Sender timestamp carried by the message. |
| `frame_id` | `std::string` | Coordinate frame or source label. |
| `received_time` | `std::chrono::system_clock::time_point` | Local SDK receive timestamp. |

### `RobotInfo`

| Member | Description |
|---|---|
| `robot_type` | Robot family, for example GR3 or GRW. |
| `hardware_type` | Hardware/model revision. |
| `end_effector_type` | Installed end-effector description. |

### `StateInfo`

| Member | Type | Description |
|---|---|---|
| `id` | `uint32_t` | FSM state identifier. |
| `version` | `uint32_t` | State contract/configuration version. |
| `name` | `std::string` | User-facing state name. |
| `description` | `std::string` | Short behavior description. |
| `active` | `bool` | Whether this entry is the active state. |

### `AuroraState`

| Member | Type | Description |
|---|---|---|
| `header` | `MessageHeader` | Message timing metadata. |
| `robot_info` | `RobotInfo` | Connected robot identity. |
| `switch_status` | `uint8_t` | Aurora state-switch status value. |
| `current_state` | `StateInfo` | Current FSM state. |
| `available_states` | `std::vector<StateInfo>` | States currently reported by Aurora. |
| `authority_resources` | `std::vector<AuthorityResourceStatus>` | Supported recognized lease resources and their advisory holder state. Missing resources are unsupported; unknown future resources are ignored. |

### `MotionState`

| Member | Type | Description |
|---|---|---|
| `header` | `MessageHeader` | Message timing metadata. |
| `position`, `position_cmd` | `std::array<double, 3>` | Measured and commanded base position. |
| `lin_velocity`, `lin_velocity_cmd` | `std::array<double, 3>` | Measured and commanded linear velocity. |
| `attitude`, `attitude_cmd` | `std::array<double, 3>` | Measured and commanded roll/pitch/yaw. |
| `ang_velocity`, `ang_velocity_cmd` | `std::array<double, 3>` | Measured and commanded angular velocity. |
| `delta_height`, `delta_height_cmd` | `double` | Measured and commanded height offset. |
| `progress` | `uint8_t` | Motion progress reported by Aurora. |
| `stable_level` | `uint8_t` | Current stability level. |

### `MotionCommand`

| Member | Type | Description |
|---|---|---|
| `header` | `MessageHeader` | Optional outgoing metadata. |
| `position` | `std::array<double, 3>` | Requested base position. |
| `lin_velocity` | `std::array<double, 3>` | Requested base-frame linear velocity. |
| `attitude` | `std::array<double, 3>` | Requested roll/pitch/yaw. |
| `ang_velocity` | `std::array<double, 3>` | Requested angular velocity. |
| `delta_height` | `double` | Requested stand-height offset. |
| `gait_type` | `uint8_t` | Robot-specific gait selector. |
| `skills` | `std::array<uint8_t, 8>` | Robot-specific skill bytes. |
| `duration` | `double` | Command validity duration in seconds. |

### `JointControlType`

`Joint` interprets vector elements as logical joints; `Motor` interprets them
as hardware motors.

### `MotorMode`

`Pd`, `Position`, and `Effort` select the command mode for a `JointCommand`.

### `JointField`

`Position`, `Velocity`, and `Effort` select the vector returned by joint
projection getters.

### `ControlGroupStateEntry`

| Member | Description |
|---|---|
| `group_name` | Aurora control-group name. |
| `joint_position` | Position vector for the group. |
| `joint_velocity` | Velocity vector for the group. |
| `joint_effort` | Effort/torque vector for the group. |

### `CartesianStateEntry`

| Member | Type | Description |
|---|---|---|
| `link_name` | `std::string` | URDF link name. |
| `pose` | `std::array<double, 7>` | Position and XYZW quaternion. |
| `twist` | `std::array<double, 6>` | Linear and angular velocity. |
| `wrench` | `std::array<double, 6>` | Force and torque. |

### `CartesianCommand`

| Member | Type | Description |
|---|---|---|
| `link_name` | `std::string` | Opaque, caller-defined lookup key; empty is invalid. |
| `pose` | `std::vector<double>` | Optional raw seven-element pose payload. |
| `twist` | `std::vector<double>` | Optional raw six-element twist payload. |
| `wrench` | `std::vector<double>` | Optional raw six-element wrench payload. |

At least one payload must be supplied. Empty vectors mean “not commanded.” Aurora stores the values unchanged; the caller and consuming task own frame semantics.

### `ControlGroupState`

| Member | Description |
|---|---|
| `header` | Message timing metadata. |
| `joint_state` | Measured joint entries. |
| `joint_cmd` | Joint commands observed by Aurora. |
| `cartesian_state` | Measured Cartesian entries for configured URDF links. |
| `cartesian_cmd` | Latest observed sparse `CartesianCommand` entries. |

### `JointCommand`

| Member | Type | Description |
|---|---|---|
| `group_name` | `std::string` | Target control group. |
| `control_type` | `JointControlType` | Joint- or motor-indexed vectors. |
| `motor_mode` | `MotorMode` | Requested control mode. |
| `position` | `std::vector<double>` | Position command vector. |
| `velocity` | `std::vector<double>` | Velocity command vector. |
| `effort` | `std::vector<double>` | Feed-forward effort command vector. |

### `ControlGroupCommand`

`header` is outgoing metadata; `joint_cmd` is the list of `JointCommand`
entries and `cartesian_cmd` is the list of sparse `CartesianCommand` entries.
Every targeted joint resource must be covered by the active lease. A non-empty
`cartesian_cmd` additionally requires `AuroraResource::CartesianCommand`.

### `BaseData`

| Member | Description |
|---|---|
| `header` | Message timing metadata. |
| `quat_xyzw`, `quat_wxyz` | Base orientation in both quaternion orderings. |
| `rpy` | Roll, pitch, yaw. |
| `omega_W`, `acc_W` | Angular velocity and linear acceleration in the world frame. |
| `omega_B`, `acc_B` | Angular velocity and linear acceleration in the base frame. |
| `vel_W`, `pos_W` | Base velocity and position in the world frame. |
| `vel_B` | Base velocity in the base frame. |
| `stable_level` | Estimator stability level. |

Quaternion fields are `std::array<double, 4>`; vector fields are
`std::array<double, 3>`.

### `ContactData`

| Member | Description |
|---|---|
| `header` | Message timing metadata. |
| `contact_force` | Per-contact estimated force values. |
| `contact_prob` | Per-contact probability values. |

### `EstimatorData`

Contains every `BaseData` payload field plus `contact_force` and
`contact_prob`: `header`, `quat_xyzw`, `quat_wxyz`, `rpy`, `omega_W`, `acc_W`,
`omega_B`, `acc_B`, `vel_W`, `pos_W`, `vel_B`, `contact_force`,
`contact_prob`, and `stable_level`.

### `GroupOverrideParam`

| Member | Description |
|---|---|
| `group_name` | Control group receiving an override. |
| `control_param` | Controller-specific numeric parameter vector. |

### `ControllerParam`

| Member | Description |
|---|---|
| `controller_name` | Universal Controller sub-controller name. |
| `rate` | Global blend/activation rate. |
| `group_override_params` | Per-group override list. |

### `SubControllerCommand`

| Member | Description |
|---|---|
| `header` | Outgoing metadata. |
| `reset` | Requests reset of Universal Controller sub-controllers. |
| `controller_params` | Controller settings to apply when `reset` is false. |

### `ErrorCodeEntry`

`high32` and `low32` are the upper and lower 32-bit halves of one Aurora error
code.

### `ErrorCodes`

| Member | Description |
|---|---|
| `header` | Message timing metadata. |
| `source` | Error-source identifier. |
| `error_codes` | Active `ErrorCodeEntry` values. |

## Lease types

### `AuroraResource`

Values: `LeftLeg`, `RightLeg`, `Waist`, `Head`, `LeftManipulator`,
`RightManipulator`, `LeftEndEffector`, `RightEndEffector`, `MotionCommand`, and
`CartesianCommand`.

### `LeasePriority`

`Normal` is the default priority. `Emergency` requires both client opt-in and
Aurora authority permission.

### `LeaseState`

| Value | Description |
|---|---|
| `Unregistered` | The module has no lease. |
| `Registering` | Initial registration is in progress. |
| `Active` | The lease is locally usable until its deadline. |
| `Replacing` | An atomic resource-set replacement is in progress. |
| `Releasing` | Release and publication drain are in progress. |
| `Lost` | The SDK cannot safely use the former lease. |

### `LeaseLossReason`

| Value | Description |
|---|---|
| `None` | No loss is recorded. |
| `DeadlineExpired` | The steady-clock lease deadline elapsed. |
| `AuthorityRejected` | Aurora rejected the lease/renew operation. |
| `OperationTimeout` | A public lease operation exceeded its timeout. |
| `InvalidAuthorityResponse` | Aurora returned an unusable response. |
| `PublicationDrainTimeout` | Protected publications did not drain in time. |
| `InternalError` | An internal lease-management failure occurred. |

### `LeaseOption`

| Member | Type | Default | Description |
|---|---|---:|---|
| `duration` | `std::chrono::milliseconds` | `4000ms` | Requested lease period. Active leases renew automatically. |
| `priority` | `std::optional<LeasePriority>` | unset | Optional explicit priority; unset means normal. |
| `timeout` | `std::chrono::milliseconds` | `500ms` | Register/replace operation deadline. |

### `LeaseResult`

| Member | Description |
|---|---|
| `operation` | `OperationResult` for the API operation. |
| `state` | Current local `LeaseState`. |
| `source_name` | Module identity attached to the lease. |
| `lease_id` | Authority-issued lease identifier. |
| `resources` | Complete leased resource set. |
| `priority` | Granted/requested priority. |
| `granted_duration` | Duration granted by Aurora. |
| `remaining_duration` | Instantaneous local time remaining. |
| `loss_reason` | Reason for `Lost`, otherwise `None`. |
| `success()` | True when `operation.success()` is true. |
| `usable()` | True only for an active lease with positive remaining duration. |

### `AuthorityResourceStatus`

| Member | Description |
|---|---|
| `resource` | Resource being described. |
| `acquired` | Whether Aurora reports a holder. |
| `holder_source_name` | Holder identity. |
| `holder_priority` | Holder lease priority. |

This state is advisory; command authorization uses the SDK-managed lease. Entries are decoded by resource name in canonical order. Missing recognized resources are allowed, duplicate recognized resources are rejected, and unknown future resource names are ignored with a throttled warning.

## Motor-configuration types

This API is temporary while the hardware SDK is unavailable and will move to
the hardware-level SDK.

### `MotorConfigurationMode`

Values: `Disable`, `Position`, `Velocity`, `Torque`, and `Pd`.

### `MotorConfigurationGroupCommand`

| Member | Description |
|---|---|
| `group_name` | Target control group. |
| `mode` | Requested `MotorConfigurationMode`. |
| `position_kp`, `position_ki`, `position_kd` | Position-loop gains. |
| `pd_kp`, `pd_kd` | PD gains. |

### `MotorConfigurationCommand`

`header` is outgoing metadata; `groups` contains the per-group commands.

### `MotorConfigurationGroupState`

| Member | Description |
|---|---|
| `group_name` | Reported control group. |
| `mode` | Current `MotorConfigurationMode`. |
| `motor_error` | Per-motor error values. |
| `position_kp`, `position_ki`, `position_kd` | Reported position-loop gains. |
| `pd_kp`, `pd_kd` | Reported PD gains. |

### `MotorConfigurationState`

`header` is message metadata; `groups` contains the reported group states.

## Callback handler types

These aliases define the callback signatures accepted by the corresponding
`AuroraClient::on...` functions.

| Type | Declaration | Payload |
|---|---|---|
| `AuroraStateHandler` | `std::function<void(AuroraState)>` | Latest Aurora/FSM/authority state. |
| `MotionStateHandler` | `std::function<void(MotionState)>` | Latest motion state. |
| `ControlGroupStateHandler` | `std::function<void(ControlGroupState)>` | Latest joint and Cartesian state. |
| `MotorConfigurationStateHandler` | `std::function<void(MotorConfigurationState)>` | Latest motor-configuration state. |
| `EstimatorDataHandler` | `std::function<void(EstimatorData)>` | Latest estimator data. |
| `ErrorCodesHandler` | `std::function<void(ErrorCodes)>` | Latest asynchronous error codes. |
| `LeaseChangedHandler` | `std::function<void(LeaseResult)>` | Updated local lease snapshot. |

Pass an empty handler to clear a callback. Register, replace, or clear handlers
only while the client is stopped.

## Client functions

### Client lifecycle

#### `getInstance`

Declaration:

```cpp
static AuroraClient& getInstance();
```

- Description: Gets the process-wide singleton without starting DDS.
- Parameters: None.
- Returns: A reference to the process-wide `AuroraClient`.

#### `config`

Declaration:

```cpp
ConnectionOptions config() const;
```

- Description: Reads the canonical connection configuration.
- Parameters: None.
- Returns: A copy of `ConnectionOptions`.

#### `configure`

Declaration:

```cpp
void configure(const ConnectionOptions& options);
```

- Description: Replaces the complete connection configuration while stopped.
- Parameters: `options` — configuration copied into the client.
- Returns: Nothing. Invalid options or lifecycle state raise an SDK exception.

#### `setDomainId`

Declaration:

```cpp
void setDomainId(uint32_t domain_id);
```

- Description: Convenience setter for `ConnectionOptions::domain_id`; updates the canonical stopped-stage configuration used by the next session.
- Parameters: `domain_id` — DDS domain identifier.
- Returns: Nothing. The client must be stopped.

#### `domainId`

Declaration:

```cpp
uint32_t domainId() const;
```

- Description: Convenience getter for the canonical `ConnectionOptions::domain_id` value.
- Parameters: None.
- Returns: The configured domain identifier.

#### `state`

Declaration:

```cpp
ClientState state() const;
```

- Description: Reads the local client lifecycle state.
- Parameters: None.
- Returns: The current `ClientState`.

#### `start`

Declaration:

```cpp
void start();
```

- Description: Validates and freezes configuration, then starts enabled DDS endpoints.
- Parameters: None.
- Returns: Nothing. Fatal startup failures raise an SDK exception.

#### `stop`

Declaration:

```cpp
void stop();
```

- Description: Drains admitted work, stops DDS, and clears session caches. Repeated calls are safe.
- Parameters: None.
- Returns: Nothing. Fatal shutdown failures raise an SDK exception.

### Endpoint readiness

#### `endpointMatchStatus`

Declaration:

```cpp
EndpointMatchStatus endpointMatchStatus() const;
```

- Description: Snapshots enabled and matched status for every endpoint.
- Parameters: None.
- Returns: An `EndpointMatchStatus` snapshot.

#### `isFsmServiceMatched`

Declaration:

```cpp
bool isFsmServiceMatched() const;
```

- Description: Checks whether the enabled FSM service has a server match.
- Parameters: None.
- Returns: `true` when the FSM service is matched.

#### `waitForEndpoints`

Declaration:

```cpp
bool waitForEndpoints(std::chrono::milliseconds timeout);
```

- Description: Waits until every enabled endpoint is matched, then waits another 200 ms so volatile publishers can deliver their first sample. Match state is checked again before success. While discovery is pending, the SDK prints a match summary once per second; it prints an all-matched summary on success and an error with matched and unmatched endpoint names on timeout.
- Parameters: `timeout` — maximum discovery wait duration; the 200 ms post-match delay is additional.
- Returns: `true` when all endpoints match, otherwise `false` on timeout.

### State access

All getters return an `OperationResult`. Atomic getters read one complete topic
snapshot; projected getters derive their result from exactly one snapshot.

#### `getAuroraState`

Declaration:

```cpp
OperationResult getAuroraState(AuroraState& output) const;
```

- Description: Reads the latest complete Aurora/FSM/authority snapshot.
- Parameters: `output` — receives the snapshot on success.
- Returns: Operation status; failure leaves `output` unchanged.

#### `getMotionState`

Declaration:

```cpp
OperationResult getMotionState(MotionState& output) const;
```

- Description: Reads the latest complete motion-state snapshot.
- Parameters: `output` — receives the snapshot on success.
- Returns: Operation status; failure leaves `output` unchanged.

#### `getControlGroupState`

Declaration:

```cpp
OperationResult getControlGroupState(ControlGroupState& output) const;
```

- Description: Reads the latest joint and Cartesian state snapshot.
- Parameters: `output` — receives the snapshot on success.
- Returns: Operation status; failure leaves `output` unchanged.

#### `getMotorConfigurationState`

Declaration:

```cpp
OperationResult getMotorConfigurationState(MotorConfigurationState& output) const;
```

- Description: Reads the latest temporary motor-configuration state.
- Parameters: `output` — receives the snapshot on success.
- Returns: Operation status; failure leaves `output` unchanged.

#### `getEstimatorData`

Declaration:

```cpp
OperationResult getEstimatorData(EstimatorData& output) const;
```

- Description: Reads the latest complete estimator snapshot.
- Parameters: `output` — receives the snapshot on success.
- Returns: Operation status; failure leaves `output` unchanged.

#### `getErrorCodes`

Declaration:

```cpp
OperationResult getErrorCodes(ErrorCodes& output) const;
```

- Description: Reads the latest asynchronous error-code snapshot.
- Parameters: `output` — receives the snapshot on success.
- Returns: Operation status; failure leaves `output` unchanged.

#### `getBaseData`

Declaration:

```cpp
OperationResult getBaseData(BaseData& output) const;
```

- Description: Projects base data from one estimator snapshot.
- Parameters: `output` — receives the projected data on success.
- Returns: Operation status; failure leaves `output` unchanged.

#### `getContactData`

Declaration:

```cpp
OperationResult getContactData(ContactData& output) const;
```

- Description: Projects contact data from one estimator snapshot.
- Parameters: `output` — receives the projected data on success.
- Returns: Operation status; failure leaves `output` unchanged.

#### `getStableLevel`

Declaration:

```cpp
OperationResult getStableLevel(uint8_t& output) const;
```

- Description: Projects the stability level from one motion snapshot.
- Parameters: `output` — receives the stability level on success.
- Returns: Operation status; failure leaves `output` unchanged.

#### `getStandPose`

Declaration:

```cpp
OperationResult getStandPose(std::vector<double>& output) const;
```

- Description: Projects `[height, pitch, yaw, stable_level]` from one motion snapshot.
- Parameters: `output` — receives the four-element vector on success.
- Returns: Operation status; failure leaves `output` unchanged.

#### `getJointState`

Declaration:

```cpp
OperationResult getJointState(const std::string& group_name, JointField field, std::vector<double>& output) const;
```

- Description: Selects one measured joint field from a named control group.
- Parameters: `group_name` — target control group; `field` — position, velocity, or effort; `output` — receives the selected vector.
- Returns: Operation status; failure leaves `output` unchanged.

#### `getJointCommand`

Declaration:

```cpp
OperationResult getJointCommand(const std::string& group_name, JointField field, std::vector<double>& output) const;
```

- Description: Selects one commanded joint field observed by Aurora.
- Parameters: `group_name` — target control group; `field` — position, velocity, or effort; `output` — receives the selected vector.
- Returns: Operation status; failure leaves `output` unchanged.

#### `getCartesianState`

Declaration:

```cpp
OperationResult getCartesianState(const std::string& link_name, CartesianStateEntry& output) const;
```

- Description: Selects measured Cartesian state by URDF link name.
- Parameters: `link_name` — requested URDF link; `output` — receives its state.
- Returns: Operation status; failure leaves `output` unchanged.

#### `getCartesianCommand`

Declaration:

```cpp
OperationResult getCartesianCommand(const std::string& link_name, CartesianCommand& output) const;
```

- Description: Selects the latest observed sparse Cartesian command by its opaque key.
- Parameters: `link_name` — exact lookup key; `output` — receives its raw command.
- Returns: Operation status; failure leaves `output` unchanged.

The `CartesianStateEntry` output overload remains available for source compatibility; omitted fields become zero arrays. New code should use `CartesianCommand` so field absence is preserved.

#### `waitForFsmState`

Declaration:

```cpp
bool waitForFsmState(uint32_t state_id, std::chrono::milliseconds timeout);
```

- Description: Waits until cached Aurora state reports the requested FSM ID.
- Parameters: `state_id` — desired FSM ID; `timeout` — maximum wait duration.
- Returns: `true` when observed, otherwise `false` on timeout.

### Local command caches

These are local intent caches, not robot execution feedback, and are cleared by
`stop()`.

#### `getLastMotionCommand`

Declaration:

```cpp
OperationResult getLastMotionCommand(MotionCommand& output) const;
```

- Description: Reads the last motion command published by this client.
- Parameters: `output` — receives the cached command on success.
- Returns: Operation status; failure leaves `output` unchanged.

#### `getLastControlGroupCommand`

Declaration:

```cpp
OperationResult getLastControlGroupCommand(ControlGroupCommand& output) const;
```

- Description: Reads the last control-group command published by this client.
- Parameters: `output` — receives the cached command on success.
- Returns: Operation status; failure leaves `output` unchanged.

#### `getLastMotorConfigurationCommand`

Declaration:

```cpp
OperationResult getLastMotorConfigurationCommand(MotorConfigurationCommand& output) const;
```

- Description: Reads the last motor-configuration command published by this client.
- Parameters: `output` — receives the cached command on success.
- Returns: Operation status; failure leaves `output` unchanged.

#### `getLastSubControllerCommand`

Declaration:

```cpp
OperationResult getLastSubControllerCommand(SubControllerCommand& output) const;
```

- Description: Reads the last successfully sent sub-controller command.
- Parameters: `output` — receives the cached command on success.
- Returns: Operation status; failure leaves `output` unchanged.

### Commands and services

Publish success means DDS accepted the sample, not that the robot executed it.

#### `publishMotionCommand`

Declaration:

```cpp
OperationResult publishMotionCommand(const MotionCommand& command);
```

- Description: Publishes one complete motion command using the active `MotionCommand` lease.
- Parameters: `command` — motion command to publish.
- Returns: Publication or lease-check status.

#### `publishControlGroupCommand`

Declaration:

```cpp
OperationResult publishControlGroupCommand(const ControlGroupCommand& command);
```

- Description: Publishes joint and/or sparse Cartesian commands after checking all required resources.
- Parameters: `command` — complete control-group command.
- Returns: Publication or lease-check status.

#### `publishMotorConfigurationCommand`

Declaration:

```cpp
OperationResult publishMotorConfigurationCommand(const MotorConfigurationCommand& command);
```

- Description: Publishes temporary motor gains after checking targeted joint resources.
- Parameters: `command` — complete motor-configuration command.
- Returns: Publication or lease-check status.

#### `setSubController`

Declaration:

```cpp
OperationResult setSubController(const SubControllerCommand& command);
```

- Description: Requests a Universal Controller sub-controller update without a lease.
- Parameters: `command` — reset or controller parameter request.
- Returns: Aurora's service result; fails when no Universal Controller task is active.

#### `requestFsmState`

Declaration:

```cpp
OperationResult requestFsmState(uint32_t state_id);
OperationResult requestFsmState(uint32_t state_id, bool force_switch);
```

- Description: Requests one Aurora FSM state without a lease.
- Parameters: `state_id` — requested FSM state ID; `force_switch` — skip the
  target tasks' `enter_check()` calls while retaining all lifecycle checks.
- Returns: Aurora's detailed FSM admission result. An already-active state is
  returned as success; accepted requests still complete asynchronously.

#### `setVelocity`

Declaration:

```cpp
OperationResult setVelocity(double vx, double vy, double yaw, double duration, uint8_t gait_type = 0);
```

- Description: Builds and publishes a base-frame velocity `MotionCommand`.
- Parameters: `vx`, `vy` — linear velocity in m/s; `yaw` — yaw rate in rad/s; `duration` — command validity in seconds; `gait_type` — robot-specific gait selector.
- Returns: Publication or lease-check status.

#### `setStandPose`

Declaration:

```cpp
OperationResult setStandPose(double delta_height, double delta_pitch, double delta_yaw, double duration = 1.0);
```

- Description: Builds and publishes nominal-stand pose offsets.
- Parameters: `delta_height` — height offset in metres; `delta_pitch`, `delta_yaw` — angular offsets in radians; `duration` — command validity in seconds.
- Returns: Publication or lease-check status.

#### `resetUnicontroller`

Declaration:

```cpp
OperationResult resetUnicontroller();
```

- Description: Sends a sub-controller request with `reset=true`.
- Parameters: None.
- Returns: Aurora's sub-controller service result.

### Lease management

Protected publications pause during replacement/release. Active leases renew
automatically. Lease state is not persisted to disk.

#### `registerLease`

Declaration:

```cpp
LeaseResult registerLease(const std::vector<AuroraResource>& resources, const LeaseOption& option = {});
```

- Description: Registers the module's first complete leased resource set.
- Parameters: `resources` — unique resources to control; `option` — requested duration, priority, and timeout.
- Returns: `LeaseResult` containing Aurora's decision and current local lease snapshot.

#### `replaceLease`

Declaration:

```cpp
LeaseResult replaceLease(const std::vector<AuroraResource>& resources, const LeaseOption& option = {});
```

- Description: Atomically requests a new complete resource set and new lease ID. A definitive rejection retains a still-valid old lease.
- Parameters: `resources` — replacement resource set; `option` — requested duration, priority, and timeout.
- Returns: `LeaseResult` for the replacement and resulting local lease.

#### `releaseLease`

Declaration:

```cpp
LeaseResult releaseLease();
```

- Description: Releases the active module lease; repeated release is safe.
- Parameters: None.
- Returns: The resulting local `LeaseResult`.

#### `getLease`

Declaration:

```cpp
LeaseResult getLease() const;
```

- Description: Reads the current module lease without contacting Aurora.
- Parameters: None.
- Returns: An instantaneous local `LeaseResult` snapshot.

### Callbacks

Register, replace, or clear callbacks while stopped. State callbacks run
inline after cache update on the DDS callback thread; lease notifications are
serialized by the SDK notification executor. Keep callbacks bounded and do
not call `stop()` from them.

#### `onAuroraState`

Declaration:

```cpp
void onAuroraState(AuroraStateHandler callback);
```

- Description: Sets or clears the post-cache-update Aurora-state callback.
- Parameters: `callback` — receives `AuroraState`; pass an empty handler to clear it.
- Returns: Nothing. The client must be stopped.

#### `onMotionState`

Declaration:

```cpp
void onMotionState(MotionStateHandler callback);
```

- Description: Sets or clears the motion-state callback.
- Parameters: `callback` — receives `MotionState`; an empty handler clears it.
- Returns: Nothing. The client must be stopped.

#### `onControlGroupState`

Declaration:

```cpp
void onControlGroupState(ControlGroupStateHandler callback);
```

- Description: Sets or clears the control-group-state callback.
- Parameters: `callback` — receives `ControlGroupState`; an empty handler clears it.
- Returns: Nothing. The client must be stopped.

#### `onMotorConfigurationState`

Declaration:

```cpp
void onMotorConfigurationState(MotorConfigurationStateHandler callback);
```

- Description: Sets or clears the motor-configuration-state callback.
- Parameters: `callback` — receives `MotorConfigurationState`; an empty handler clears it.
- Returns: Nothing. The client must be stopped.

#### `onEstimatorData`

Declaration:

```cpp
void onEstimatorData(EstimatorDataHandler callback);
```

- Description: Sets or clears the estimator-data callback.
- Parameters: `callback` — receives `EstimatorData`; an empty handler clears it.
- Returns: Nothing. The client must be stopped.

#### `onErrorCodes`

Declaration:

```cpp
void onErrorCodes(ErrorCodesHandler callback);
```

- Description: Sets or clears the asynchronous error-code callback.
- Parameters: `callback` — receives `ErrorCodes`; an empty handler clears it.
- Returns: Nothing. The client must be stopped.

#### `onLeaseChanged`

Declaration:

```cpp
void onLeaseChanged(LeaseChangedHandler callback);
```

- Description: Sets or clears the serialized lease-change callback.
- Parameters: `callback` — receives `LeaseResult`; an empty handler clears it.
- Returns: Nothing. The client must be stopped.

## Result helpers

| Function | Description |
|---|---|
| `std::string_view resultCodeName(uint32_t) noexcept` | Returns the stable symbolic name, or `UNKNOWN_RESULT_CODE`. |
| `std::string_view defaultResultMessage(uint32_t) noexcept` | Returns the default English message for a known code. |
| `OperationResult::success()` | Tests `code == SUCCESS`. |
| `LeaseResult::success()` | Delegates to `operation.success()`. |
| `LeaseResult::usable()` | Tests active state and positive remaining duration. |
