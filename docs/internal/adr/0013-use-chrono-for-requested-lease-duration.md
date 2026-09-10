---
status: accepted
---

# Use chrono for requested lease duration

C++ registration and replacement options represent requested lease duration as
`std::chrono::milliseconds`, defaulting to `4000 ms`. Python accepts a finite
positive number of seconds and converts it to the native duration.

## Consequences

Durations below the documented Aurora Stage 1 minimum of `100 ms` and
unrepresentable durations fail local validation. Python seconds round down to a
whole millisecond before validation and are never rounded upward. Requests above
Aurora's configured maximum remain valid and are clamped by the authority; the
Control Lease reports the actual granted duration.

Lease duration describes authority lifetime. It is separate from the
caller-visible timeout used to bound completion of a registration operation.
