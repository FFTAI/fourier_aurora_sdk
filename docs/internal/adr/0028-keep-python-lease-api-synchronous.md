---
status: accepted
---

# Keep the Python lease API synchronous

Python directly mirrors native `registerLease`, `replaceLease`, `releaseLease`,
`getLease`, and `onLeaseChanged` with snake-case names and the same typed enums,
options, results, states, and loss reasons. Finite positive seconds convert to
native millisecond durations.

## Consequences

The binding adds no lease context manager because the singleton client, not a
Python object scope, owns authority. It adds no separate asyncio state machine;
applications may use `asyncio.to_thread` around bounded synchronous operations.
Renewal, clocks, retry identity, callbacks, and state remain canonical native
C++ behavior shared by both languages.
