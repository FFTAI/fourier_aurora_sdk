---
status: accepted
---

# Always renew active leases

Every `Active` SDK lease renews automatically. `LeaseOption` has no
`auto_renew` field, and the public SDK exposes no manual-renew method. Users end
ownership with `releaseLease()`.

## Consequences

The SDK schedules normal renewal at one quarter of the actual granted duration,
uses an individual attempt timeout no greater than one eighth, allows one
renewal action in flight, and exactly retries its original identity and
payload. The lease remains Active while its existing Conservative Client
Deadline is valid and becomes `Lost` if renewal is not confirmed by that
deadline.

Renewal policy cannot be accidentally disabled by an application. Native fake
clock and mock transport seams provide deterministic timeout and retry tests
without exposing production timing controls.

Retry scheduling cannot busy-spin on an immediate pre-send failure: it waits for
endpoint readiness or a 10 ms monotonic retry tick. An attempt that already
consumed its bounded response timeout may retry immediately, always before the
current conservative deadline.
