---
status: accepted
---

# Use one singleton client for one robot per process

Aurora SDK `2.0` is designed for single-robot control. `AuroraClient` remains a
process-wide singleton, and multi-robot applications use separate processes or
another orchestration layer.

The singleton object has stable process lifetime. Parameterless `getInstance()`
initializes or returns the object without starting transport. `start()` creates
the configured FourierDDS endpoints, and synchronous `stop()` tears them down
while leaving the object in `Stopped` for reconfiguration and restart. The API
does not expose terminal `close()` or destroy the singleton during a method
call.

Every reference and Python wrapper is an alias of the same client. Aliases share
configuration, callbacks, caches, and lifecycle; one alias calling `stop()`
stops transport for all of them. Configuration mutation remains restricted to
`Stopped`.

This decision avoids duplicate high-level command sessions inside one process,
but it does not provide command authority across processes or DDS publishers.
Aurora remains responsible for validating and arbitrating commands from SDK,
ROS, and direct-IDL clients.
