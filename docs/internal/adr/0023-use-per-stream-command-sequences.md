---
status: accepted
---

# Use per-stream Command Sequences

The SDK owns independent telemetry-only Command Sequences for motion, Robot
Control Group, and the temporarily restored motor-configuration stream. Each
begins at zero for a fresh Lease Period, survives renewal, and resets for a
fresh `lease_id`. Aurora does not gate Stage 1 commands on this value, and raw
DDS/ROS clients may transmit zero.

## Consequences

Commands serialize sequence allocation and DDS write only within their own
stream, so different streams may publish concurrently. Local rejection and
failed DDS writes consume nothing; successful writes consume a value and update
that stream's last-command cache. Sequence exhaustion never wraps and never
rejects a Stage 1 command: the transmitted field saturates at `UINT64_MAX`, and
an observational counter/log records the condition without affecting lease
authority or publication.

The motor-configuration API mirrors the retained command/state payload and
former behavior while the hardware SDK remains unavailable. Source comments
and user documentation identify it as a temporary Aurora SDK surface intended
to move to the hardware-level SDK later, but it is not compiler-deprecated
while it remains the supported customized-gain path.
