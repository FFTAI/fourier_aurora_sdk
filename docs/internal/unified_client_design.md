# Unified Diagnostic Client Design

This document records the agreed product and command-line contract used to
replace the three installed diagnostic clients with one binary.

## Agreed Requirements

- The binary exposes every capability currently provided by `state_client`,
  `motion_client`, and `control_group_client`.
- The command line uses an `area -> command` hierarchy so identically named
  operations in different SDK interfaces remain unambiguous.
- DDS domain ID defaults to `123` and may be overridden with `-d` or
  `--domain_id`.
- Source Name defaults to a per-invocation `aurora-client-NNNN` value and may
  be overridden with `-s` or `--source_name`.
- Package installation makes the binary directly callable through the normal
  system `PATH`.
- The installed client provides shell argument completion.

## Command Shape

The installed executable is named `aurora-client`:

```text
aurora-client [global options] <area> <command> [command options]
```

The initial command areas correspond to the three existing diagnostic client
surfaces:

- `state`
- `motion`
- `control-group`

Estimator telemetry is promoted from an operation inside the old motion client
to a separate, read-only `estimator` area. This makes the new areas correspond
to distinct SDK concerns rather than preserving the old source-file boundary.

The `state` name is retained for the area backed by `AuroraState`. Although it
includes FSM operations, it also reports robot information and switch status,
so the narrower name `fsm` was rejected.

### State area

`state get` retains the complete output of the existing client's
`--get-state` operation: message header, robot type, hardware type, end-effector
type, switch status, and all current FSM fields.

The complete state surface is:

```text
state get
state list
state set <state-id>
state monitor [--interval <value>]
```

Only one state command is executed per invocation. `state set` reports the
immediate request result and does not imply that the asynchronous transition
has completed. `state monitor` retains a default polling interval of 500 ms and
allows the caller to override it with `--interval`.

Monitor intervals are positive durations. A bare value is interpreted as
milliseconds; explicit `ms` and `s` suffixes are also accepted:

```text
state monitor --interval 250
state monitor --interval 250ms
state monitor --interval 2s
```

Decimal values are accepted, including fractional seconds such as `0.5s`.
After conversion, the interval must be at least 10 ms; shorter values are
rejected as usage errors to prevent accidental busy polling.

### Estimator area

Estimator telemetry is read-only and has two commands:

```text
estimator get
estimator monitor [--interval <duration>]
```

`get` prints the complete estimator payload exposed by the existing diagnostic
client. `monitor` uses the shared 500 ms default and monitor-interval syntax.
Only one estimator command is executed per invocation.

### Motion area

The motion area exposes motion-state telemetry and single-publication motion
commands:

```text
motion get
motion monitor [--interval <duration>]
motion set [command fields]
```

`get` and `monitor` print the complete motion-state payload. Monitor behavior
uses the shared interval rules. `set` requires at least one command field,
acquires the `MotionCommand` lease, and publishes exactly once. Any supported
fields may be combined in that publication:

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

The canonical velocity names are expanded from the old `--lin-velocity` and
`--ang-velocity` spellings for clarity.

### Control-group area

The control-group area exposes complete control-group state and interpolated
joint-position commands:

```text
control-group get
control-group monitor [--interval <duration>]
control-group set \
  --target <group-name> <joint0> [joint1 ...] \
  [--target <group-name> <joint0> [joint1 ...]] \
  [--duration <seconds>] \
  [--rate <hz>]
```

`get` and `monitor` print joint, Cartesian, and commanded state. Monitor
behavior uses the shared interval rules. `set` requires at least one
repeatable `--target`; each target names a group followed by its joint values.
The client retains the existing five-second duration and 100 Hz rate defaults,
validates targets against observed control-group state, infers the required
resource leases, and interpolates all targets simultaneously.

## Global connection options

The connection overrides are recognized anywhere in the command line:

```text
aurora-client -d 7 -s state-reader state get
aurora-client state -d 7 get -s state-reader
aurora-client state get --domain_id 7 --source_name state-reader
```

Their defaults and accepted spellings are:

```text
-d, --domain_id <uint32>    default: 123
-s, --source_name <name>    default: aurora-client-NNNN
```

The four-digit suffix is generated once for each valid robot-facing process
invocation. An explicit name and the generated default both match
`[A-Za-z0-9._-]{1,64}`. Help, version, completion, and invalid argument paths
do not initialize the SDK or generate/log a Source Name.

## Installation exposure

The Debian package owns both the executable under the SDK prefix and a
system-path symbolic link:

```text
/opt/fftai/fourier_aurora_sdk/bin/aurora-client
/usr/bin/aurora-client -> /opt/fftai/fourier_aurora_sdk/bin/aurora-client
```

This makes the packaged client available to every user and lets package
removal clean up the link. A custom non-Debian CMake installation installs the
binary into its selected prefix's `bin` directory and does not mutate a second
system prefix; the installer is responsible for choosing a prefix on `PATH` or
adding it to `PATH`.

## Shell completion

The package ships completion definitions for Bash, Zsh, and Fish in their
standard system directories. Bash support is essential and receives explicit
behavioral tests. Zsh and Fish are a low-maintenance extension of the same
small command tree and receive syntax and smoke checks.

Completion covers areas, commands, valid option names, and the fixed supported
control-group names after `--target`. It may offer duration-unit hints but does
not invent numeric state IDs, motion values, or joint targets.

## Compatibility

The unified binary cleanly replaces `state_client`, `motion_client`, and
`control_group_client`. Those executables are no longer built or installed,
and no compatibility wrappers or legacy flat-argument mode are provided.
Documentation and package-content checks refer only to `aurora-client`.

## Help, version, and usage errors

Help is available at the root, area, and command levels through `-h` and
`--help`. Running the program without arguments prints root help and succeeds;
running only an area prints that area's help and succeeds. Unknown or invalid
input prints a concise error with the relevant usage and exits with the shared
usage-error code `2`.

`--version` prints the SDK/client version. Help and version handling never
starts the client or initializes DDS. Help text exposes effective defaults,
including domain ID `123`, source ID `1234`, monitor interval `500ms`, and the
control-group set defaults of `5s` and `100Hz`.

## Endpoint discovery

Robot-facing commands wait up to a fixed five seconds for all required DDS
endpoints. The timeout is not configurable in this release. Failure to match
the required endpoints is a runtime failure and identifies the five-second
timeout in its diagnostic.

## Connection diagnostics

Every robot-facing invocation prints its effective connection identity before
starting the client:

```text
domain_id=123 source_name=aurora-client-0427
```

The banner is written to standard error, leaving command results on standard
output easier to pipe or capture. Help and version do not print it.

## Output format

This release emits human-readable text only and preserves the existing field
names and complete diagnostic payloads. It does not expose JSON or another
structured-output mode. Structured output is deferred until its timestamps,
enums, missing-data representation, and streaming record contract can be
designed as a stable interface.

## Duplicate options

Supplying any singleton option more than once is a usage error, even when both
values are identical. This applies to global connection overrides and all
command options. The sole repeatable option is control-group set's
`--target`, where repetition intentionally builds a simultaneous target set.

## Process exit contract

The unified client preserves the diagnostic clients' exit-code meanings:

| Code | Meaning |
|---:|---|
| `0` | Help/version displayed, or the requested operation succeeded |
| `1` | Startup, discovery, lease, transport, or SDK operation failed |
| `2` | Command-line usage is invalid |
| `130` | The invocation was interrupted by `SIGINT` or `SIGTERM` |

Monitor commands and interrupted one-shot commands use `130` when stopped
before normal completion.

## Endpoint selection

The unified executable does not enable every SDK endpoint at startup. It
derives the minimal endpoint set from the selected command, preserving the
existing tools' least-capability and low-overhead behavior. Read commands
enable only their subscriber; write commands additionally enable only the
required lease service and publisher.
