---
status: accepted
---

# Use six public lease states

The SDK exposes `Unregistered`, `Registering`, `Active`, `Replacing`,
`Releasing`, and `Lost`. Only `Active` may be usable. Successful release returns
to `Unregistered`; there is no separate `Released` state.

## Consequences

Registration and replacement uncertainty is visible without exposing exact
retry machinery. Replacement and release stop protected commands through their
public transition states. Expiry, revocation, timeout, uncertainty, invalid
authority response, publication-drain timeout, and a native internal failure
after authority might exist converge on the unusable `Lost` state. A fully
stopped client instead discards session protocol state and reports
`Unregistered`.

Every `Lost` snapshot carries one stable `LeaseLossReason`; non-`Lost` snapshots
use `None`. The reason categories are `DeadlineExpired`, `AuthorityRejected`,
`OperationTimeout`, `InvalidAuthorityResponse`, `PublicationDrainTimeout`, and
`InternalError`. The accompanying operation result and diagnostics preserve
more specific wire or transport details.

Automatic renewal does not add a public state while the existing Conservative
Client Deadline remains valid. Retry and cleanup substates after loss remain
native SDK internals.
