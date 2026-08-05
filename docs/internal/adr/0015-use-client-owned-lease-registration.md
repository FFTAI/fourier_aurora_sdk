---
status: accepted
---

# Use client-owned lease registration

`AuroraClient` owns its one current module lease. Applications explicitly call
`registerLease()`, `replaceLease()`, and `releaseLease()`, each returning a
`LeaseResult`; the SDK does not return a user-owned `ControlLease` handle.
Protected commands take no lease argument and use the current client-owned
token only after local usability and complete resource-coverage validation.

## Consequences

Registration remains visible and never occurs as a hidden command side effect,
but applications do not distribute handles or replace references when an
atomic replacement creates a fresh Lease Period. Native C++ remains the single
owner of renewal, retries, sequences, deadlines, and raw DDS lease fields.

`LeaseResult::success()` describes the operation that just completed, while
`LeaseResult::usable()` describes the conservative lease snapshot returned
with it. A failed replacement can therefore report operation failure while the
old lease remains usable. Documentation and examples must explain this
distinction and show complete resource registration before protected commands.

`LeaseOption` defaults requested duration to 4000 ms and the total synchronous
operation timeout to 500 ms. The two durations have independent meanings.
