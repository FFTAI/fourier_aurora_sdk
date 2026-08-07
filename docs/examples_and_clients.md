# Examples and Diagnostic Clients

[中文](examples_and_clients_CN.md) · [Quick start](../README.md) ·
[Overview](overview.md) · [C++ API](cpp_api.md) · [Python API](python_api.md)

The repository contains two kinds of runnable programs:

- robot-independent C++ diagnostic clients for inspecting endpoints and
  issuing focused test operations;
- small GR3 recipes that demonstrate application structure in C++ and Python.

These programs are learning and diagnostic tools, not autonomous safety
controllers. Before running a motion example, support the robot, clear the
work area, verify the robot model and FSM IDs, and be ready to stop it.

## Where to find them

| Program | Source tree | Native build | Debian installation |
|---|---|---|---|
| Diagnostic clients | `examples/client/` | `build/{state_client,motion_client,control_group_client}` | `/opt/fftai/fourier_aurora_sdk/bin/client/` |
| C++ GR3 recipes | `examples/gr3/cpp/` | `build/gr3_{get_info,walk_forward,swing_arms,aurora_state_callback}` | Source under `share/doc/fourier_aurora_sdk/examples/gr3/cpp/` |
| Python GR3 recipes | `examples/gr3/python/` | Run from source | Source under `share/doc/fourier_aurora_sdk/examples/gr3/python/` |

Python wheels contain only the client package; they do not contain examples or
diagnostic executables.

## Before running a program

1. Start Aurora and confirm that its DDS domain and namespace match the client.
2. Confirm compatible DDS and message runtime packages are installed.
3. For protected commands, obtain a deployment-assigned, nonzero `source_id`.
4. Run read-only operations first and verify the robot type/current FSM.
5. Check every printed `OperationResult`, not only the process exit code.

The GR3 recipes use DDS domain `123`. Diagnostic clients accept a domain on the
command line.

## Diagnostic client common contract

Every tool requires a DDS domain ID and nonzero module `source_id`:

```text
<tool> <domain_id> <source_id> <operation...>
```

Both are unsigned 32-bit integers. `source_id` is the lease authority's module
identity and must be unique among concurrently connected modules. Tools print
their domain and source at startup.

Each tool creates a fixed endpoint bundle. Options select operations, not DDS
construction. The tools intentionally do not call aggregate
`waitForEndpoints()` because an unrelated endpoint should not block a focused
diagnostic. Each operation prints its exact result on failure.

| Exit code | Meaning |
|---:|---|
| `0` | All requested operations succeeded |
| `1` | Startup, transport, or operation failure |
| `2` | Invalid command-line usage |
| `130` | Interrupted by `SIGINT` or `SIGTERM` |

### `state_client`

Endpoints: `AuroraStateSubscriber`, `FsmService`.

```bash
state_client 123 42 --get-state
state_client 123 42 --get-available-states
state_client 123 42 --request-state 13
state_client 123 42 --get-state --request-state 13
state_client 123 42 --monitor-state
```

- `--get-state` prints robot information, switch status, and current FSM.
- `--get-available-states` prints Aurora's advertised state list.
- `--request-state` sends a numeric FSM request and prints Aurora's reply.
- `--monitor-state` must be used alone; it reads the latest cache every 500 ms.

### `motion_client`

Endpoints: `MotionStateSubscriber`, `EstimatorDataSubscriber`,
`MotionCommandPublisher`, `ManageLeaseService`.

Read operations:

```bash
motion_client 123 42 --get-state
motion_client 123 42 --get-estimator
motion_client 123 42 --get-state --get-estimator
motion_client 123 42 --monitor-state
motion_client 123 42 --monitor-estimator
```

A monitor option must be the only operation. Monitor mode reads the latest
cache every 500 ms and only repeats a transient failure when it changes.

Command fields may be combined into one `MotionCommand`:

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

```bash
motion_client 123 42 \
  --get-state --get-estimator \
  --lin-velocity 0.1 0.0 0.0 --duration 1.0
```

The client registers the `MotionCommand` lease before publishing. Motion
position is world-frame; linear/angular velocity is robot-base-frame. Values
use SI units.

### `control_group_client`

Endpoints: `ControlGroupStateSubscriber`, `ControlGroupCommandPublisher`,
`ManageLeaseService`.

```bash
control_group_client 123 42 --get-state
control_group_client 123 42 --monitor-state
control_group_client 123 42 \
  --position left_manipulator 0.0 0.2 0.0 -0.7 \
  --position right_manipulator 0.0 -0.2 0.0 -0.7 \
  --duration 3 --rate 100
```

`--get-state` prints latest joint and Cartesian state/command entries.
`--monitor-state` must be used alone. One or more `--position` groups linearly
interpolate from one measured snapshot to the supplied joint positions.
`--duration` defaults to 5 seconds and `--rate` to 100 Hz.

Before publishing, the client registers one lease covering every named joint
group. Aurora validates whether names, vector lengths, and modes are valid for
the connected robot.

## GR3 recipes

Build the native recipes with `FOURIER_AURORA_SDK_BUILD_EXAMPLES=ON` (the native
default). Python recipes require an installed `fourier_aurora_client` wheel or
an equivalent development package on `PYTHONPATH`.

### `aurora_state_callback`

Purpose: demonstrate read-only callback subscription without acquiring control.
It creates only `AuroraStateSubscriber`, counts every state sample, and prints
the message send time, local SDK receive time, and total count every tenth
callback. Both timestamps use `HH:MM:SS:ffffff`.

```bash
cmake --build build --target gr3_aurora_state_callback
./build/gr3_aurora_state_callback
./build/gr3_aurora_state_callback 5  # stop automatically after 5 seconds
```

The optional positive duration is useful for bounded diagnostics and CI. With
no argument, press Ctrl+C to stop. The implementation is in
`examples/gr3/cpp/aurora_state_callback.cpp`; callback signatures are listed in
the [C++ API](cpp_api.md#callback-handler-types).

### `get_info`

Purpose: verify basic GR3 connectivity and inspect its advertised configuration
without acquiring control.

This read-only recipe selects three subscribers, waits up to five seconds, and
prints robot type, hardware/end-effector type, current FSM, base orientation
and velocity, stable level, and available control-group names.

```bash
./build/gr3_get_info
python3 examples/gr3/python/get_info.py
```

Start here to verify connectivity without acquiring control.

### `walk_forward`

Purpose: demonstrate the complete bounded motion workflow: lease registration,
FSM transition, motion command, and lease release.

This bounded recipe accepts the module source ID, registers the
`MotionCommand` resource, requests GR3 Walk (FSM `3`), waits for the transition,
then commands `0.3 m/s` forward for three seconds. It requires an exact `YES`
confirmation before taking control.

```bash
./build/gr3_walk_forward 42
python3 examples/gr3/python/walk_forward.py 42
```

Review the constants in the source before use. Do not assume FSM IDs or limits
are portable to another robot type.

### `swing_arms`

Purpose: demonstrate leased multi-group joint control with interpolation from
one measured state snapshot.

This bounded recipe accepts the module source ID, leases both manipulator
resources, requests GR3 PdStandStanding (FSM `13`), reads both arm positions
from one snapshot, interpolates to a center pose over three seconds, and runs
two five-second arm-swing periods at 100 Hz. It requires `YES` confirmation.

```bash
./build/gr3_swing_arms 42
python3 examples/gr3/python/swing_arms.py 42
```

The example expects `left_manipulator` and `right_manipulator` group ordering
used by GR3. Inspect and validate joint indices against the connected robot
before running it.

## Use the examples as application templates

The examples intentionally keep each workflow in one file. When adapting one:

1. Replace hard-coded robot capabilities and FSM IDs with deployment config.
2. Select only the endpoints the application actually uses.
3. Assign a registered module `source_id`; never generate one at runtime.
4. Register the complete resource set before protected publishers.
5. Bound every wait and motion, handle interruption, and stop in all paths.
6. Treat state/error telemetry as the robot outcome; local publish is not an
   acknowledgement.

For complete method signatures, continue with the [C++ API](cpp_api.md) or
[Python API](python_api.md).
