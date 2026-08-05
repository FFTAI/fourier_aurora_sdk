---
status: accepted
---

# Expose lease snapshots and change notification

The SDK exposes `getLease()` for a fresh conservative `LeaseResult` snapshot
and `onLeaseChanged()` for optional lifecycle notification. Callback
registration occurs while `AuroraClient` is stopped and Python mirrors both
APIs.

## Consequences

Polling reports current remaining duration instead of replaying an old
operation result. Notification fires for lifecycle changes, loss, and
Active-to-Active replacement that changes the grant. Routine successful renewal
does not create callback traffic while state remains Active.

Observer snapshots normalize `operation` from current state: Unregistered and
Active use `SUCCESS`, transition states use `LEASE_OPERATION_IN_PROGRESS`, and
Lost uses `LEASE_LOST` plus its loss reason. Only the synchronous mutator return
retains that call's exact authority/transport outcome, so polling cannot replay a
stale failed operation.

Lease transitions enqueue coalesced immutable snapshots after state commit and
outside internal locks. Callbacks are serialized on a dedicated notification
executor, never the renewal/timer worker or a DDS callback thread, so a slow
application callback cannot delay renewal. Lease lifecycle operations are
invalid when invoked reentrantly from the notification callback; `getLease()` is
safe.

Notification is edge-coalesced current state, not an audit log. Intermediate
pending transitions may collapse when user code is slow. While Running, a
pending `Lost` transition is a mandatory ordering barrier and cannot be replaced
by either an older event or a newer registration; the bounded queue retains that
latest pending loss plus at most the latest following ordinary snapshot. A newer
loss replaces an older undelivered loss and intervening ordinary states, so this
remains current-state notification rather than an audit log. Stop waits only for a
currently admitted callback and discards queued callbacks. A callback caused by
a synchronous mutator may execute before or after that method returns, so the
method result is the call-completion signal. An admitted callback must return in
bounded time because synchronous stop waits for it even though renewal does not.

Python exposes this lease-notification callback as a deliberate exception to
the older transport-callback restriction. The binding acquires the GIL for
invocation and contains user exceptions. Registration persists as stopped-state
client configuration across sessions until replaced or cleared; interpreter
finalization clears the Python reference and drops queued work.
