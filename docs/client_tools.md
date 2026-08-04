# Diagnostic Client Tools

The native Linux SDK package installs three robot-independent diagnostic tools
under:

```text
/opt/fftai/fourier_aurora_sdk/bin/client/
```

These programs are for SDK and Aurora debugging. They are C++ tools, are not
included in Python wheels, and do not encode a robot-specific workflow.

## Common Contract

Every tool requires the DDS domain ID and a nonzero module `source_id`:

```text
<tool> <domain_id> <source_id> <operation...>
```

Both values are unsigned 32-bit integers. `source_id` is sent to Aurora as the
module identity and is used by the lease authority. It must be stable and
unique among concurrently connected modules. Both values are printed at
startup.

Each tool always creates its documented endpoint bundle. Options select
operations, not endpoint construction. The tools intentionally do not call
`waitForEndpoints()` because one unrelated endpoint should not block
diagnostics; every operation prints its exact `OperationResult` on failure.

Process exit codes are:

| Code | Meaning |
|---:|---|
| `0` | All requested operations succeeded |
| `1` | Startup, transport, or operation failure |
| `2` | Invalid command-line usage |
| `130` | Interrupted by `SIGINT` or `SIGTERM` |

Signal handlers only set a flag. Normal control flow leaves the operation,
calls `stop()` synchronously, and then exits.

## `state_client`

Fixed endpoints:

- `AuroraStateSubscriber`
- `FsmService`

Usage:

```bash
state_client 123 42 --get-state
state_client 123 42 --get-available-states
state_client 123 42 --request-state 13
state_client 123 42 --get-state --request-state 13
state_client 123 42 --monitor-state
```

`--get-state` prints the latest robot information, switch status, and current
FSM state. `--get-available-states` prints the complete advertised state list.
`--request-state` sends an arbitrary numeric FSM request and prints Aurora's
reply without remapping it. `--monitor-state` must be used alone and reads the
latest cached snapshot every 500 ms.

## `motion_client`

Fixed endpoints:

- `MotionStateSubscriber`
- `EstimatorDataSubscriber`
- `MotionCommandPublisher`
- `ManageLeaseService`

The tool registers the `motion_cmd` lease before publishing a command and
fails without publishing if registration is unsuccessful.

Read options:

```text
--get-state
--monitor-state
--get-estimator
--monitor-estimator
```

One-shot `--get-state` and `--get-estimator` may be combined. A monitor option
must be the only operation. Monitor mode reads the latest SDK snapshot every
500 ms, prints complete snapshots, reports only changes in transient failure
results, and resumes snapshot output when data becomes available.

`--get-estimator` and `--monitor-estimator` print every `EstimatorData` field:
both quaternion orders, RPY, world/base angular velocity and acceleration,
world/base linear velocity, world position, contact force/probability, header,
and stable level.

Command fields may be combined to publish one `MotionCommand`:

```text
--position <x> <y> <z>
--lin-velocity <vx> <vy> <vz>
--attitude <roll> <pitch> <yaw>
--ang-velocity <wx> <wy> <wz>
--delta-height <value>
--gait-type <uint8>
--skills <s0> <s1> <s2> <s3> <s4> <s5> <s6> <s7>
--duration <seconds>
```

Example:

```bash
motion_client 123 42 \
  --get-state --get-estimator \
  --lin-velocity 0.1 0.0 0.0 --duration 1.0
```

All numeric values use SI units. Motion position is in the world frame; linear
and angular velocity are in the robot base frame. Local publish success is not
Aurora execution acknowledgement.

## `control_group_client`

Fixed endpoints:

- `ControlGroupStateSubscriber`
- `ControlGroupCommandPublisher`
- `ManageLeaseService`

Usage:

```bash
control_group_client 123 42 --get-state
control_group_client 123 42 --monitor-state
control_group_client 123 42 \
  --position left_manipulator 0.0 0.2 0.0 -0.7 \
  --position right_manipulator 0.0 -0.2 0.0 -0.7 \
  --duration 3 --rate 100
```

`--get-state` prints the latest joint and Cartesian state/command entries.
`--monitor-state` must be used alone and polls the latest snapshot every
500 ms. One or more `--position` groups linearly interpolate from one
`ControlGroupState` snapshot to the supplied joint positions. `--duration`
defaults to 5 seconds and `--rate` defaults to 100 Hz.

The SDK forwards representable group names, vector lengths, control modes, and
numeric values. Aurora remains responsible for robot-specific semantic
validation. Before publishing, the tool registers one lease covering every
joint group in the command.
