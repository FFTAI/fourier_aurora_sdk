---
status: superseded by ADR-0009 and ADR-0015
---

# Use explicit SDK-managed Control Leases

> Superseded by ADR 0009, which replaces multiple independent leases with one
> module lease, and ADR 0015, which moves that lease onto `AuroraClient` instead
> of returning a user-held object.

The high-level Aurora SDK requires applications to acquire a declared resource
set explicitly and returns an SDK-managed Control Lease object. Applications
bind that object explicitly to protected commands and release it explicitly;
command publication never acquires or reacquires authority as a hidden side
effect. The native SDK automatically owns renewal by default, exact management
retries, sequence handling, and conservative deadline enforcement so C++ and
Python clients do not reproduce the failure-sensitive wire protocol.

## Consequences

One application may hold several independent objects for disjoint resource
sets, but one command binds exactly one usable object and never combines their
coverage. Losing a lease disables its command binding and is observable to the
application; it does not trigger implicit reacquisition. The canonical state
and timers live in C++, while Python aliases the same native objects.

Object ownership, destruction fallback, renewal controls, and concrete command
signatures are specified separately and must preserve explicit acquisition and
binding.
