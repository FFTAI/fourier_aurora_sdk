---
status: superseded by ADR-0010
---

# Use one global Acquisition Sequence counter per Source ID

> Superseded by ADR 0010. With no durable SDK acquisition counter, there is no
> persistent counter namespace to configure or migrate.

The SDK persists one increasing Acquisition Sequence counter for each numeric
`source_id`, shared across all Aurora authority addresses used by that logical
source. This is a safe refinement of the wire protocol's per-authority scope
because gaps are permitted, and it prevents DDS domain, namespace, or endpoint
alias changes from selecting a lower local counter.

## Consequences

The store does not need a derived or deployment-supplied Lease Authority key.
Every new acquisition from the module is serialized through the Source ID's
counter; already-granted leases retain independent Lease-Action and command
sequence spaces.

Moving one logical source to another machine or state directory requires
moving its durable counter state. Reusing the Source ID from an unrelated
store is not a supported recovery mechanism.
