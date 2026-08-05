---
status: accepted
---

# Use a typed Aurora resource API

The public C++ SDK represents the nine Stage 1 Aurora authority resources with
`enum class AuroraResource : uint8_t`, mirrored by a Python enum. Acquisition
and replacement accept only these typed values. The native SDK owns conversion
to the canonical DDS resource strings and serializes resources in receiver
catalog order.

## Consequences

Misspelled or unknown resource strings cannot reach the lease service through
the high-level SDK. Invalid cast values and duplicate resources are rejected
locally before transport. C++ and Python share the same closed resource
universe and coverage rules.

A future Aurora resource requires an SDK update before applications can request
it. The SDK receives no arbitrary-string escape hatch because it must also
understand how every exposed resource maps to protected command validation.
