---
status: accepted
---

# Wait for the former lease to expire after SDK restart

A restarted SDK begins without authority and immediately starts a fresh lease
registration under its stable module Source ID and new memory-only Client
Instance ID. If Aurora still has an active Lease Period for that Source ID, it
returns `SOURCE_ALREADY_ACTIVE` without changing or disclosing the former
token. Rejected registration creates no replay record. Within one
`registerLease()` call, the new SDK waits with a fixed 50 ms retry cadence and
resubmits the same logical registration with the same Operation ID and payload;
it never adopts or supersedes the former lease.

The cadence is a minimum between attempt start times and attempts never overlap.
An attempt lasting more than 50 ms may be followed immediately on completion if
the operation budget remains.

## Consequences

Knowledge of a Source ID alone cannot revoke valid authority. A duplicate live
process configured with the same Source ID also cannot steal the holder's
lease; it remains unauthorized while the holder renews.

Restart recovery may wait up to the former lease's remaining duration, which is
bounded by the Aurora receiver profile. Because the rejection is not cached,
the same operation may be reevaluated and granted after ordinary expiry. Public
registration respects its caller-visible timeout and never waits indefinitely.
If the call returns, a later explicit `registerLease()` is a new logical
operation with a fresh Operation ID.
