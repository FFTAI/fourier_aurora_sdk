---
status: accepted
---

# Treat lease-operation timeout as abandonment

`LeaseOption::timeout` defaults to 500 ms and bounds the complete synchronous
`registerLease()` or `replaceLease()` operation. If the SDK cannot establish a
definitive safe outcome by that boundary, the call returns failure and the
client lease state becomes `Lost`.

## Consequences

A timed-out operation never becomes usable later in the background. Native
code may continue only bounded exact resolution and cleanup; discovery of a
late successful grant triggers best-effort release while the SDK remains
unusable. Regaining authority requires another explicit `registerLease()`.

The native transport distinguishes a provable `NotSent` attempt from
`OutcomeUnknown` after DDS handoff. Registration with only pre-send failures
remains `Unregistered`; replacement with no authority request restores a
still-usable old grant. Publication-drain timeout is different: a stuck admitted
writer makes continuation unsafe even though replacement is not sent. Release is
locally irreversible and remains unusable until absence is confirmed.

The timeout does not alter the requested or granted lease duration. A restarted
client waiting through a former lease's possible five-second lifetime must
explicitly select a longer operation timeout or retry registration after the
default responsive 500 ms call returns.
