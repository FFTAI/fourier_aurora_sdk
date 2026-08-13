# Aurora SDK

The Aurora SDK context covers the public client used to configure and operate
an Aurora robot through a transport session.

## Language

**Running client**:
An Aurora client whose local transport endpoints are active. It does not imply
that an Aurora robot has been discovered or is reachable.
_Avoid_: Connected client, ready client

**Stopped client**:
An Aurora client with no active transport session. Its configuration may be
changed and it may be started again.
_Avoid_: Closed client

**Restartable transport session**:
The finite DDS-resource lifetime created by one successful `start()` and ended
by `stop()` or fatal cleanup. Starting again creates fresh transport resources
without replacing the process-wide client object.
_Avoid_: Client instance, permanent connection

**SDK-local user label**:
A nonempty process diagnostic label configured through `setUserId()`. The
current protocol does not send it to Aurora and it provides no authentication
or routing.
_Avoid_: Username, robot identity, credential

**Single-robot process**:
An application process whose one Aurora client controls one robot. Multi-robot
control uses separate processes or an external orchestration layer.
_Avoid_: Multi-session client

**Endpoint match status**:
Whether a local DDS endpoint has discovered its required remote counterpart.
It is observable only while the client is running and does not by itself prove
that the robot application is healthy or ready to execute commands.
_Avoid_: Client state, robot readiness

**Enabled endpoint**:
An SDK communication endpoint selected for creation in the next or current
transport session. Endpoints not selected are absent rather than unmatched.
_Avoid_: Connected endpoint, initialized endpoint

**Configured endpoint set**:
The canonical endpoint list stored in `ConnectionOptions`. It defaults to all
endpoints, is frozen for each transport session, and is replaced as part of a
complete stopped-stage configuration update.
_Avoid_: Runtime operation flags

**Operation result**:
The immediate outcome of an SDK getter, setter, publish, or RPC request. It can
originate inside the client or from an Aurora reply and is distinct from
asynchronous robot fault telemetry.
_Avoid_: ErrorCodes, exception

**Estimator telemetry**:
Read-only robot pose, motion, inertial, contact, and stability observations
published by Aurora's state estimator.
_Avoid_: Motion state, estimator command

**Client diagnostic tool**:
A robot-independent command-line program for inspecting or exercising Aurora
SDK client interfaces. It accepts robot-dependent identifiers or values as
runtime input rather than encoding a robot workflow.
_Avoid_: Robot example, Python example

**Robot recipe**:
A short, self-contained illustration of one concrete workflow for a named robot
type. One workflow may contain multiple ordered operations when they serve one
clear user goal. A recipe belongs to one robot type even when another robot
currently uses identical behavior.
_Avoid_: Generic client, shared example framework

**Base frame**:
The body-fixed robot frame whose positive x axis points forward, positive y
points left, and positive z points upward.

**World frame**:
The fixed frame in which motion-command position is expressed. It is distinct
from the body-fixed base frame used by motion linear and angular velocity.

**Nominal standing attitude**:
Aurora's reference standing orientation from which motion-command roll, pitch,
and yaw offsets are measured.
