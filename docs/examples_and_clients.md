# Examples and Diagnostic Clients

For GR3-Mini, start with the [onboard environment guide and Python/C++ examples](../examples/gr3mini/README.md). The Python example covers state reading, walking, and head control; the C++ example is read-only.

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
| Diagnostic client | `examples/client/aurora_client.cpp` | `build/aurora-client` | `aurora-client` on the system `PATH` |
| C++ GR3 recipes | `examples/gr3/cpp/` | `build/gr3_{get_info,walk_forward,swing_arms,aurora_state_callback}` | Source under `share/doc/fourier_aurora_sdk/examples/gr3/cpp/` |
| Python GR3 recipes | `examples/gr3/python/` | Run from source | Source under `share/doc/fourier_aurora_sdk/examples/gr3/python/` |

Python wheels contain only the client package; they do not contain examples or
diagnostic executables.

## Before running a program

1. Start Aurora and confirm that its DDS domain and namespace match the client.
2. Confirm compatible DDS and message runtime packages are installed.
3. Choose a human-readable `source_name` matching `[A-Za-z0-9._-]{1,64}`.
4. Run read-only operations first and verify the robot type/current FSM.
5. Check every printed `OperationResult`, not only the process exit code.

The GR3 recipes and diagnostic client default to DDS domain `123`.

## Diagnostic client common contract

The unified diagnostic executable uses an area and command hierarchy:

```text
aurora-client [global options] <area> <command> [command options]
```

DDS domain ID defaults to `123`. Override it anywhere on the command line with
`-d`/`--domain_id`. The wrapper generates a process-local default Source Name
`aurora-client-NNNN`; override it with `-s`/`--source_name` when a stable,
descriptive name is useful. Source Names match `[A-Za-z0-9._-]{1,64}`. The
client prints the effective domain and Source Name to standard error at startup.

```bash
aurora-client state get
aurora-client -d 7 -s state-reader state get
aurora-client state get --domain_id 7 --source_name state-reader
```

The client enables only the endpoints required by the selected command. It
waits up to five seconds for those endpoints to match before reading state,
sending a request, or registering a lease. Read-only get and monitor operations
do not enable lease management, register a lease, or claim control resources.
Each operation prints its exact result on failure.

| Exit code | Meaning |
|---:|---|
| `0` | All requested operations succeeded |
| `1` | Startup, transport, or operation failure |
| `2` | Invalid command-line usage |
| `130` | Interrupted by `SIGINT` or `SIGTERM` |

Run `aurora-client --help`, `aurora-client <area> --help`, or
`aurora-client <area> <command> --help` without starting DDS. The Debian
package also installs Bash, Zsh, and Fish completion definitions.

Monitor commands default to `500ms`. `--interval` accepts milliseconds as a
bare value or explicit `ms`/`s` suffixes; the minimum is `10ms`.

### `state`

Endpoints are selected by operation: read and monitor operations use
`AuroraStateSubscriber`; `set` uses `FsmService`.

```bash
aurora-client state get
aurora-client state list
aurora-client state set 13
aurora-client state set 13 --force
aurora-client state monitor
aurora-client state monitor --interval 250ms
```

- `get` prints robot information, switch status, and current FSM.
- `list` prints Aurora's advertised state list.
- `set` sends a numeric FSM request and prints Aurora's immediate reply.
  `--force` skips target-task `enter_check()` calls only.
- `monitor` reads the latest cache until interrupted.

### `estimator`

Estimator telemetry is a separate read-only area:

```bash
aurora-client estimator get
aurora-client estimator monitor --interval 1s
```

It prints the complete estimator pose, motion, inertial, contact, and stability
payload. Read-only invocations do not enable lease management or claim a
control resource.

### `motion`

Motion-state reads use `MotionStateSubscriber`. Command publication uses
`MotionCommandPublisher` plus `ManageLeaseService`.

```bash
aurora-client motion get
aurora-client motion monitor --interval 500
```

Command fields may be combined into one `MotionCommand`:

```text
--position <x> <y> <z>
--linear-velocity <vx> <vy> <vz>
--attitude <roll> <pitch> <yaw>
--angular-velocity <wx> <wy> <wz>
--delta-height <value>
--gait-type <uint8>
--skills <s0> <s1> <s2> <s3> <s4> <s5> <s6> <s7>
--duration <seconds>
```

```bash
aurora-client motion set \
  --linear-velocity 0.1 0.0 0.0 --duration 1.0
```

`motion set` requires at least one field, publishes exactly once, and
registers the `MotionCommand` lease after its selected endpoints match.
Read-only invocations remain passive and can monitor Aurora while another
source owns the motion lease.
Motion position is world-frame; linear/angular velocity is robot-base-frame.
Values use SI units.

### `control-group`

Read and monitor operations use only `ControlGroupStateSubscriber`. A `set`
also enables `ControlGroupCommandPublisher` and
`ManageLeaseService`.

```bash
aurora-client control-group get
aurora-client control-group monitor
aurora-client control-group set \
  --target left_manipulator 0.0 0.2 0.0 -0.7 \
  --target right_manipulator 0.0 -0.2 0.0 -0.7 \
  --duration 3 --rate 100
```

`get` prints latest joint and Cartesian state/command entries. One or more
repeatable `--target` values linearly interpolate from one measured snapshot
to the supplied joint positions.
`--duration` defaults to 5 seconds and `--rate` to 100 Hz.

Before publishing, and only after its selected endpoints match, the client
registers one lease covering every named joint group. Read-only invocations do
not register a lease. Aurora validates whether names, vector lengths, and modes
are valid for the connected robot.

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
./build/gr3_aurora_state_callback state-callback
./build/gr3_aurora_state_callback state-callback 5  # stop after 5 seconds
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
./build/gr3_get_info info-reader
python3 examples/gr3/python/get_info.py info-reader
```

Start here to verify connectivity without acquiring control.

### `walk_forward`

Purpose: demonstrate the complete bounded motion workflow: lease registration,
FSM transition, motion command, and lease release.

This bounded recipe accepts the module Source Name, registers the
`MotionCommand` resource, requests GR3 Walk (FSM `3`), waits for the transition,
then commands `0.3 m/s` forward for three seconds. It requires an exact `YES`
confirmation before taking control.

```bash
./build/gr3_walk_forward walk-demo
python3 examples/gr3/python/walk_forward.py walk-demo
```

Review the constants in the source before use. Do not assume FSM IDs or limits
are portable to another robot type.

### `swing_arms`

Purpose: demonstrate leased multi-group joint control with interpolation from
one measured state snapshot.

This bounded recipe accepts the module Source Name, leases both manipulator
resources, requests GR3 PdStandStanding (FSM `13`), reads both arm positions
from one snapshot, interpolates to a center pose over three seconds, and runs
two five-second arm-swing periods at 100 Hz. It requires `YES` confirmation.

```bash
./build/gr3_swing_arms arm-demo
python3 examples/gr3/python/swing_arms.py arm-demo
```

The example expects `left_manipulator` and `right_manipulator` group ordering
used by GR3. Inspect and validate joint indices against the connected robot
before running it.

### `sub_controller` (Python)

Purpose: demonstrate how arm passthrough commands interact with Universal
Controller runtime rates and reset.

The recipe accepts the module Source Name, leases both manipulators, and requests
GR3 ExternalUniController (FSM `120`). It first disables momentum and waist
compensation and commands both arms to zero. After confirmation, it commands
both elbow joints to `-1.0`, sets momentum to `0.5` and waist compensation to
`1.0`, then waits for confirmation before resetting Universal Controller. Each
arm transition starts from the latest measured state and interpolates to its
target over two seconds at 100 Hz.

```bash
python3 examples/gr3/python/sub_controller.py arm-passthrough
```

This example assumes the GR3 seven-joint manipulator ordering, with the elbow
at index `3`.

## Use the examples as application templates

The examples intentionally keep each workflow in one file. When adapting one:

1. Replace hard-coded robot capabilities and FSM IDs with deployment config.
2. Select only the endpoints the application actually uses.
3. Assign a readable, distinct module `source_name`.
4. Register the complete resource set before protected publishers.
5. Bound every wait and motion, handle interruption, and stop in all paths.
6. Treat state/error telemetry as the robot outcome; local publish is not an
   acknowledgement.

For complete method signatures, continue with the [C++ API](cpp_api.md) or
[Python API](python_api.md).
