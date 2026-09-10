---
status: superseded by ADR-0010
---

# Let the SDK own Acquisition Sequence persistence

> Superseded by ADR 0010. The SDK now keeps lease-management retry state only
> in memory and starts every run without authority or locally restored protocol
> state.

The native C++ SDK owns the durable Acquisition Sequence store used by Managed
Control Leases. It atomically reserves and persists a sequence before the first
network transmission of each new logical acquisition; a crash may consume a
sequence and leave a gap, but cannot cause reuse. Python uses the same native
store, and ordinary public APIs cannot supply, decrement, or reset the
counter.

## Consequences

Failure to durably reserve a sequence prevents the acquisition request from
being sent. Only acquisition ordering is persisted: lease IDs, Lease-Action
Sequences, deadlines, and command authority are always discarded on restart.
An internal store abstraction supports fake storage and failure injection in
tests without making applications implement persistence callbacks.

The durable location, counter namespace, locking, corruption policy, and
recovery tooling remain explicit API/deployment decisions.
