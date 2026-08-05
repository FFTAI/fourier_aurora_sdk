---
status: accepted
---

# Use one replaceable lease per module

One module-level Source ID may own at most one active lease at an Aurora Lease
Authority. The SDK manages that single module lease automatically and exposes
an atomic resource-replacement operation. A definitive replacement rejection
leaves the old grant unchanged; successful replacement terminates the old Lease
Period and creates a new one with a fresh `lease_id`.

## Consequences

Applications do not coordinate several lease handles or combine authority when
sending a multi-resource command. Changing resources is one authority-side
transaction rather than an availability-check/release/request sequence.

The fresh token prevents delayed commands carrying the old token from becoming
authorized for resources added by replacement. Replacement success resets the
new lease's command sequence spaces. An uncertain response pauses
protected publication because the SDK cannot know whether Aurora has committed
the token switch. Exact retries may resolve it only before the public operation
deadline; after that abandonment boundary, a late success is cleanup-only.

All module resources share one renewal and loss lifecycle. Under ADR 0030,
replacement, renewal, and release each use an opaque Operation ID while the SDK
keeps at most one logical management operation in flight. Exact replacement
replay is retained under the old token and Operation ID. Priority,
resources, and duration form the desired atomic new grant; priority defaults to
the old value but may be changed explicitly when permitted for the Source ID.
Replacement success is usable only when processed before both the public
operation deadline and its own first-send-based deadline, even if the old
deadline passed; rejection resumes the old token only before the operation
deadline while its old deadline remains usable. If an unusably late success
reveals a fresh token, the SDK remains `Lost`, publishes no protected commands,
and performs a bounded best-effort release before allowing another registration
attempt.
