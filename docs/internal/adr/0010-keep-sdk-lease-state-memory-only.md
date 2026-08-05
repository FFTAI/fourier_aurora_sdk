---
status: accepted
---

# Keep SDK lease state memory-only

Every `AuroraClient` run begins without authority and without restored lease
protocol state. The SDK writes no lease ID, Management Operation ID, deadline,
retry record, or related metadata to local storage. A
restarted client performs a fresh explicit lease registration and may use only the
fresh Lease Period returned for that run.

Retry identity needed while the process is alive is generated
and retained internally in memory. On each successful `start()`, native C++
generates a random nonzero Client Instance ID and starts its non-wrapping
Operation ID allocator at `1`. Neither is user-supplied. Both are discarded by
`stop()` or process termination. Under ADR 0011, a previous
still-active Lease Period is never adopted or superseded by the restarted SDK.

## Consequences

The SDK has no lease-state directory, persistence callback, file locking,
corruption recovery, or state-migration API. C++ and Python expose the same
clean-start behavior.

A restarted client never renews, releases, replaces, or publishes commands
with the former token. Aurora may still retain that old Lease Period until
expiry because it cannot infer process death from missing local state. Fresh
registrations receive `SOURCE_ALREADY_ACTIVE` until ordinary expiry permits a new
grant.
