---
status: accepted
---

# Map specific authority results

Aurora acknowledges each processed ManageLease operation with the contract's
most specific result: OK, invalid request, unknown resource, priority denied,
resource busy, lease not found, lease expired, Operation ID conflict, authority
error, source already active, or bounded-authority backpressure. The SDK maps
these to stable lease-specific public codes outside its existing result-code
range. In particular, `AUTHORITY_BUSY` maps to `LEASE_AUTHORITY_BUSY` and remains
distinct from `RESOURCE_BUSY` so callers can identify transient bookkeeping
capacity rather than ownership contention.

The SDK reserves public result codes `300..399` for the lease/sub-controller
extension. Wire lease failures map in order to `300..309`; local lease outcomes
use `310..314`. Existing code `0`, client/transport range `100..199`, and Aurora
service range `200..299` remain unchanged. The exact assignments live in the API
specification and `result_codes.md`.

## Consequences

Programming and configuration misuse throws `InvalidArgumentError`; expected
runtime, transport, arbitration, lifecycle, and authority outcomes return
`LeaseResult` or `OperationResult`. Python uses the same boundary. Malformed
authority responses use `INVALID_AURORA_RESPONSE` and force conservative loss.

Public release treats already-absent or expired authority as successful while
retaining the wire outcome for diagnostics. Protected command topics remain
one-way and do not acknowledge receiver admission; SDK success means DDS write,
not Aurora execution. Contract tests cover every authority result and mapping.
