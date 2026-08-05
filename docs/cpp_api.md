# C++ API Reference

[中文](cpp_api_CN.md) · [Quick start](../README.md) ·
[Overview](overview.md) · [Python API](python_api.md)

The supported high-level C++ API is declared by:

```cpp
#include "fourier_aurora_sdk/aurora_client.hpp"
namespace sdk = fourier::aurora_sdk;
```

All public types are in `fourier::aurora_sdk`. The library requires C++17.

## Calling conventions

- `AuroraClient` is a process-wide singleton; use `getInstance()`.
- Configure the client and callbacks only while its state is `Stopped`.
- `start()` and `stop()` manage local transport. Call `stop()` explicitly.
- Getters and commands return `OperationResult`; expected operation failure is
  not an exception.
- A failed getter leaves its output parameter unchanged.
- Timeout parameters use `std::chrono::milliseconds` and accept compatible
  durations such as `std::chrono::seconds(5)`.
- Numeric robot values use SI units unless a field says otherwise.

## Client configuration and lifecycle

| Function | Return | Purpose |
|---|---|---|
| `AuroraClient::getInstance()` | `AuroraClient&` | Return the singleton without starting DDS |
| `config()` | `ConnectionOptions` | Return a copy of the current configuration |
| `configure(options)` | `void` | Replace the complete configuration while stopped |
| `setDomainId(id)` | `void` | Set the DDS domain for the next session; default is `123` |
| `domainId()` | `uint32_t` | Read the configured DDS domain |
| `state()` | `ClientState` | Read local lifecycle state |
| `start()` | `void` | Freeze configuration and start transport |
| `stop()` | `void` | Drain admitted work and synchronously stop transport |
| `endpointMatchStatus()` | `EndpointMatchStatus` | Return enabled/matched flags for all endpoints |
| `isFsmServiceMatched()` | `bool` | Check the enabled FSM service match directly |
| `waitForEndpoints(timeout)` | `bool` | Wait until every enabled endpoint is matched |

`ClientState` values are `Stopped`, `Starting`, `Running`, and `Stopping`.
`Running` is local state, not robot readiness.

### `ConnectionOptions`

| Field | Default | Meaning |
|---|---|---|
| `namespace_name` | unset | Optional DDS namespace override |
| `dds_mode` | `0` | FourierDDS mode; valid values are `0..3` |
| `is_ros_compatible` | unset | Optional ROS-compatibility override |
| `use_namespace` | `true` | Enable namespaced endpoints |
| `use_discovery_server` | unset | Optional discovery-server override |
| `service_timeout_ms` | `1000` | Default synchronous service timeout |
| `source_id` | `0` | Deployment-assigned module identity for lease use |
| `source_name` | unset | Diagnostic name; defaults to `aurora_sdk_<source_id>` |
| `allow_emergency_lease_priority` | `false` | Explicitly allow this client to request emergency priority |
| `enabled_endpoints` | all endpoints | Exact endpoints created by `start()` |

`source_id` must be nonzero when lease management or a protected publisher is
enabled. A protected publisher also requires `ManageLeaseService`.

Available `Endpoint` values are:

```text
FsmService                    ManageLeaseService
MotionCommandPublisher        ControlGroupCommandPublisher
MotorConfigurationCommandPublisher
SubControllerService
AuroraStateSubscriber         MotionStateSubscriber
ControlGroupStateSubscriber   MotorConfigurationStateSubscriber
EstimatorDataSubscriber       ErrorCodesSubscriber
```

## State getters

Atomic getters return the latest complete cache for one DDS state topic.

| Function | Output | Required endpoint |
|---|---|---|
| `getAuroraState(output)` | `AuroraState` | `AuroraStateSubscriber` |
| `getMotionState(output)` | `MotionState` | `MotionStateSubscriber` |
| `getControlGroupState(output)` | `ControlGroupState` | `ControlGroupStateSubscriber` |
| `getMotorConfigurationState(output)` | `MotorConfigurationState` | `MotorConfigurationStateSubscriber` |
| `getEstimatorData(output)` | `EstimatorData` | `EstimatorDataSubscriber` |
| `getErrorCodes(output)` | `ErrorCodes` | `ErrorCodesSubscriber` |

Focused getters derive a view from one cached topic snapshot:

| Function | Output / behavior | Source snapshot |
|---|---|---|
| `getBaseData(output)` | Base pose, velocity, acceleration, orientation, stable level | `EstimatorData` |
| `getContactData(output)` | Contact forces and probabilities | `EstimatorData` |
| `getStableLevel(output)` | Stable level | `MotionState` |
| `getStandPose(output)` | `[delta_height, pitch, yaw, stable_level]` | `MotionState` |
| `getJointState(group, field, output)` | Measured position, velocity, or effort | `ControlGroupState.joint_state` |
| `getJointCommand(group, field, output)` | Observed commanded position, velocity, or effort | `ControlGroupState.joint_cmd` |
| `getCartesianState(link, output)` | Measured pose, twist, and wrench for a URDF link | `ControlGroupState.cartesian_state` |
| `getCartesianCommand(link, output)` | Observed Cartesian command for a link | `ControlGroupState.cartesian_cmd` |
| `waitForFsmState(id, timeout)` | Wait until the cached FSM ID equals `id` | `AuroraState` |

`JointField` is `Position`, `Velocity`, or `Effort`. Cartesian state is
read-only in this release; there is no Cartesian command publisher API.

Client-cache getters return what this client last published or successfully
sent, not robot-reported execution state:

| Function | Output |
|---|---|
| `getLastMotionCommand(output)` | `MotionCommand` |
| `getLastControlGroupCommand(output)` | `ControlGroupCommand` |
| `getLastMotorConfigurationCommand(output)` | `MotorConfigurationCommand` |
| `getLastSubControllerCommand(output)` | `SubControllerCommand` |

These caches are cleared when a session stops.

## Commands and services

| Function | Endpoint | Lease | Meaning |
|---|---|---|---|
| `publishMotionCommand(command)` | `MotionCommandPublisher` | `MotionCommand` | Publish one complete robot motion command |
| `publishControlGroupCommand(command)` | `ControlGroupCommandPublisher` | Every targeted joint resource | Publish joint commands for named control groups |
| `publishMotorConfigurationCommand(command)` | `MotorConfigurationCommandPublisher` | Every targeted joint resource | Publish customized motor gains/configuration |
| `setSubController(command)` | `SubControllerService` | none | Ask the active Universal Controller task to apply/reset sub-controller configuration |
| `requestFsmState(state_id)` | `FsmService` | none | Request an Aurora FSM state |

Motor configuration is temporarily exposed here because the hardware SDK is
not yet available; it will move to the hardware-level SDK. Aurora performs
robot-specific group-name, vector-length, mode, FSM, and capability validation.

Convenience setters construct a complete command and delegate to the functions
above:

| Function | Constructed command |
|---|---|
| `setVelocity(vx, vy, yaw, duration, gait_type=0)` | Base-frame linear X/Y velocity and yaw rate in `MotionCommand` |
| `setStandPose(delta_height, delta_pitch, delta_yaw, duration=1.0)` | Height/pitch/yaw stand offsets in `MotionCommand` |
| `resetUnicontroller()` | `SubControllerCommand` with `reset=true` |

`setSubController()` fails if the current active task does not contain a
Universal Controller. Command publication success is not execution
acknowledgement.

## Control lease API

| Function | Purpose |
|---|---|
| `registerLease(resources, option={})` | Register the module's first complete resource set |
| `replaceLease(resources, option={})` | Atomically replace the current complete resource set |
| `releaseLease()` | Release the current lease; repeated release is safe |
| `getLease()` | Return an instantaneous local `LeaseResult` snapshot |

`AuroraResource` values are `LeftLeg`, `RightLeg`, `Waist`, `Head`,
`LeftManipulator`, `RightManipulator`, `LeftEndEffector`, `RightEndEffector`,
and `MotionCommand`.

`LeaseOption` contains:

| Field | Default | Meaning |
|---|---:|---|
| `duration` | `2000ms` | Requested lease period; active leases renew automatically |
| `priority` | unset (`Normal`) | Optional `Normal` or `Emergency` request |
| `timeout` | `500ms` | Overall public register/replace operation deadline |

Emergency priority additionally requires
`allow_emergency_lease_priority=true`; Aurora remains the authority that may
accept or reject it.

`LeaseResult` includes `operation`, `state`, `source_id`, `source_name`,
`lease_id`, `resources`, `priority`, `granted_duration`, `remaining_duration`,
and `loss_reason`. `success()` reports the operation result. `usable()` is the
stronger command condition: the state is `Active` and remaining duration is
positive.

```cpp
sdk::LeaseOption lease_option;
lease_option.duration = std::chrono::seconds(2);
lease_option.timeout = std::chrono::milliseconds(500);

auto lease = client.registerLease(
    {sdk::AuroraResource::LeftManipulator,
     sdk::AuroraResource::RightManipulator},
    lease_option);
if (!lease.usable()) {
    std::cerr << lease.operation.code << ": "
              << lease.operation.message << "\n";
}
```

During replacement and release, protected publishers are paused. A definitive
replacement rejection restores the old lease if it is still valid; an
uncertain timeout may leave the local lease `Lost`. Lease state is never saved
to disk.

## Callbacks

Register or replace callbacks while the client is stopped:

| Function | Callback argument |
|---|---|
| `onAuroraState(callback)` | `AuroraState` |
| `onMotionState(callback)` | `MotionState` |
| `onControlGroupState(callback)` | `ControlGroupState` |
| `onMotorConfigurationState(callback)` | `MotorConfigurationState` |
| `onEstimatorData(callback)` | `EstimatorData` |
| `onErrorCodes(callback)` | `ErrorCodes` |
| `onLeaseChanged(callback)` | `LeaseResult` |

State callbacks run inline after the DDS cache update. Keep them bounded,
non-blocking, and exception-safe, and do not call `stop()` from a callback.
Lease notifications are serialized on the SDK notification executor.

## Data types

| Area | Main types and fields |
|---|---|
| Common | `MessageHeader{send_time, frame_id, received_time}`, `OperationResult` |
| Robot/FSM | `RobotInfo`, `StateInfo`, `AuroraState{robot_info, switch_status, current_state, available_states, authority_resources}` |
| Motion | `MotionCommand{position, lin_velocity, attitude, ang_velocity, delta_height, gait_type, skills[8], duration}`, corresponding measured/commanded `MotionState` fields |
| Control group | `ControlGroupStateEntry`, `CartesianStateEntry{link_name, pose[7], twist[6], wrench[6]}`, `JointCommand`, `ControlGroupCommand` |
| Estimator | `BaseData`, `ContactData`, `EstimatorData` with world (`_W`) and base (`_B`) frame fields |
| Motor configuration | `MotorConfigurationGroupCommand/State`, `MotorConfigurationCommand/State`, `MotorConfigurationMode` |
| Sub-controller | `GroupOverrideParam`, `ControllerParam`, `SubControllerCommand` |
| Errors | `ErrorCodeEntry{high32, low32}`, `ErrorCodes{source, error_codes}` |
| Lease authority | `LeaseOption`, `LeaseResult`, `AuthorityResourceStatus` and lease enums |

Fixed-size pose arrays use `[x, y, z, qx, qy, qz, qw]`. Motion position is in
the world frame; motion velocity and Cartesian state are in the robot base
frame unless Aurora's robot-specific contract states otherwise.

## Results and exceptions

`OperationResult::success()` is true only for code `0`. Use
`resultCodeName(code)` and `defaultResultMessage(code)` for SDK-known codes.

| Range | Owner |
|---:|---|
| `0` | Success |
| `100..199` | Local SDK client/transport result |
| `200..299` | Aurora service result, passed through |
| `300..319` | Lease result |
| `320..339` | Sub-controller result |

Important local failures include `CLIENT_NOT_RUNNING`, `ENDPOINT_DISABLED`,
`ENDPOINT_UNMATCHED`, `PUBLISH_FAILED`, `SERVICE_TIMEOUT`, `TRANSPORT_ERROR`,
`INVALID_AURORA_RESPONSE`, and `STATE_NOT_RECEIVED`. Lease failures additionally
distinguish invalid requests, busy resources, expiry, operation timeout, lost
leases, and insufficient resource coverage.

The exception hierarchy is:

```text
std::runtime_error
└── AuroraSdkError
    ├── ConnectionError
    ├── InvalidArgumentError
    └── InvalidStateError
```

Catch `InvalidArgumentError` for invalid configuration/arguments,
`InvalidStateError` for lifecycle misuse, and `ConnectionError` for fatal
transport startup or teardown failures.

## Low-level transport facade

`transport/fourierdds_transport.hpp` installs a concrete
`FourierDdsTransport`. It is the advanced transport surface used to integrate
the SDK core, not the recommended application client.

| Category | Functions |
|---|---|
| Construction | `FourierDdsTransport(domain_id, options={})`, destructor |
| Receive handlers | `setAuroraStateHandler`, `setMotionStateHandler`, `setControlGroupStateHandler`, `setMotorConfigurationStateHandler`, `setEstimatorDataHandler`, `setErrorCodesHandler` |
| Aggregate status | `endpointMatchStatus()` |
| Service status | `isFsmServiceMatched()`, `isManageLeaseServiceMatched()`, `isSubControllerServiceMatched()` |
| Publisher status | `isMotionCommandPublisherMatched()`, `isControlGroupCommandPublisherMatched()`, `isMotorConfigurationCommandPublisherMatched()` |
| Subscriber status | `isAuroraStateSubscriberMatched()`, `isMotionStateSubscriberMatched()`, `isControlGroupStateSubscriberMatched()`, `isMotorConfigurationStateSubscriberMatched()`, `isEstimatorDataSubscriberMatched()`, `isErrorCodesSubscriberMatched()` |
| Lease-exempt services | `requestFsmState(state_id)`, `setSubController(command)` |
| Shutdown | `stop()` |
| SDK-core integration | `publishMotionCommand`, `publishControlGroupCommand`, `publishMotorConfigurationCommand`, `manageLease` |

The SDK-core integration functions deliberately require types from the
`detail` namespace. Applications cannot safely create those lease tokens or
management requests and must use the corresponding `AuroraClient` functions.
Use the facade directly only when maintaining the SDK transport integration.
