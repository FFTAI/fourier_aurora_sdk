# Command Contract

Aurora SDK command values use SI units unless an API explicitly states
otherwise.

## Units

| Value | Unit |
|---|---|
| Revolute joint position | radians |
| Revolute joint velocity | radians per second |
| Revolute joint effort | newton-meters |
| Prismatic joint position | meters |
| Prismatic joint velocity | meters per second |
| Prismatic joint effort | newtons |
| Cartesian translation | meters |
| Cartesian quaternion | dimensionless `[qx, qy, qz, qw]`, normalized |
| Cartesian linear twist | meters per second |
| Cartesian angular twist | radians per second |
| Cartesian force | newtons |
| Cartesian torque | newton-meters |
| Motion-command duration | seconds |

SDK examples must use these units explicitly in variable names, comments, or
nearby explanatory text. Robot-specific limits remain defined by Aurora and
the robot configuration rather than by generic SDK constants.

## Validation Ownership

The SDK is a transport wrapper. It does not decide whether group names, vector
dimensions, control modes, gait values, numeric ranges, or command combinations
are valid for the active robot. Aurora owns this validation so SDK, ROS, and
direct-IDL publishers have the same command semantics.

A successful SDK publish result means that the wrapper published the command;
it does not by itself mean that Aurora accepted or executed it. Aurora must
reject malformed or unsupported commands defensively regardless of their
source.

Aurora reports command rejection asynchronously through the `ErrorCodes`
endpoint. The current protocol does not guarantee that an error can be
correlated with a specific publication. Applications requiring confirmation
must monitor error telemetry and robot state; this SDK release provides no
per-command acknowledgement.

## FSM State IDs

FSM requests use numeric state IDs. Aurora reserves `12` for
`PdStandHanging` and `13` for `PdStandStanding` as default states with the same
meaning on every supported robot. Examples using these IDs must show the state
name in the constant, comment, and user-visible output. Other available states
remain discoverable through `AuroraState.available_states` and may vary with
the robot configuration.

FSM ID `3` is the GR3 Walk state, not a cross-robot default. Robot-specific
recipes live under `examples/<robot-type>/{cpp,python}`; current recipes are
under `examples/gr3/`. Each recipe is standalone and belongs to one robot type,
even when another robot currently uses identical source. Applications must not
reuse robot-specific IDs for another robot type without checking that robot's
capability documentation.

Robot-independent C++ diagnostic tools live directly under `examples/client/`.
They may share internal CLI helpers and accept explicit connection and operation
arguments; they are distinct from short, hardcoded robot recipes.

`requestFsmState(id)` only confirms that Aurora accepted the transition
request. The transition itself is asynchronous. Applications can use
`waitForFsmState(id, timeout)` to observe `AuroraState.current_state.id`, and
command examples must wait for the required state before publishing motion.

In C++, the timeout is `std::chrono::milliseconds`; Python accepts seconds.
The wait returns `true` after observing the requested ID and `false` on timeout,
client stop, a disabled `AuroraState` endpoint, or absence of a matching state
sample. Waiting never sends a transition request.

The SDK does not block publication based on cached FSM state. Examples must
state their required FSM state, wait for it, and abort if it is not observed.
Aurora still validates the precondition when processing every command.

## Reference Frames

The initial SDK uses fixed command frames. Callers cannot select arbitrary
frames, and the SDK does not claim to transform command values between frames.
The fixed frame for each supported command field is documented below.

Motion linear and angular velocity use the robot base frame. Its positive x
axis points forward, positive y points left, and positive z points upward.
Positive yaw rate is counterclockwise about positive z.

Motion position uses the world frame. Position and velocity therefore use
different fixed frames within the same motion command; callers must not apply
one command-wide frame interpretation.

Motion attitude is `[roll, pitch, yaw]` in radians, expressed as offsets from
Aurora's nominal standing attitude. `[0, 0, 0]` means no attitude offset; it is
not an absolute world orientation.

## Cartesian Scope

Cartesian state pose and twist are read-only and expressed in the robot base
frame. Cartesian command publishing is not supported in the first public SDK
because the corresponding Aurora command path is not implemented.

The DDS IDL retains `cartesian_cmd` as a reserved field for future protocol
compatibility. Its presence in the wire schema does not mean that clients may
use it. A later release must define controller, frame, and failure semantics
before exposing Cartesian pose, twist, or wrench commands.
