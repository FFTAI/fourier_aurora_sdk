---
status: accepted
---

# Validate complete command lease coverage locally

Protected SDK methods use the one client-owned current lease. Motion commands
require MotionCommand authority; Robot Control Group and motor-configuration
payloads require every uniquely targeted canonical group resource.

## Consequences

Unknown groups, `whole_body`, malformed payloads, non-Active authority, expired
local deadlines, and incomplete coverage fail before DDS publication. Local
rejection consumes no Command Sequence and does not change the last-published
command cache. Only a successful DDS write updates that cache.

Lease admission remains guarded through the DDS write, so replacement, release,
or loss cannot invalidate a checked token before publication completes. SDK
success confirms publication only, not Aurora admission or downstream command
execution.
