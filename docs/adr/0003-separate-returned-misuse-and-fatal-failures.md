---
status: accepted
---

# Separate returned, misuse, and fatal failures

Expected operation failures return `OperationResult` without SDK logging; API
misuse throws a specific exception without SDK logging; fatal failures that
make continued operation unsafe are logged and thrown after moving the client
to `Stopped` where possible. This keeps transient robotics conditions out of
exception control flow while preserving strong signals for programming errors
and corrupted client state.

The public C++ and Python exception hierarchy is intentionally small:

- `AuroraSdkError` is the common SDK base.
- `InvalidArgumentError` reports invalid API or configuration values.
- `InvalidStateError` reports operations prohibited by lifecycle or callback
  context.
- `ConnectionError` reports transport startup, shutdown, or fatal connection
  failure.

Endpoint readiness, missing state, publication failure, and service timeout use
`OperationResult`; they do not have exception classes. `CallbackError` is also
unnecessary because callback-context misuse is an `InvalidStateError`.

Python mirrors this hierarchy exactly. `AuroraSdkError` derives from Python
`RuntimeError`, and each specific SDK exception derives only from
`AuroraSdkError`.

Exceptions raised by C++ user callbacks are caught and logged at the
asynchronous callback boundary because there is no synchronous caller to
receive them. They do not stop the client or prevent later callback delivery.
Scope-bound callback bookkeeping guarantees that synchronous `stop()` cannot
hang after an exception. Python transport-thread callback registration is not
exposed in stage one because it would add GIL and interpreter-lifecycle risk
without a required Python use case. ADR 0020 defines one deliberate exception:
lease-change notification runs on a dedicated native notification executor,
not a DDS or renewal thread, and has explicit GIL/finalization handling.

A fatal operation invoked from a C++ callback first rejects new work and queues
teardown away from the callback thread. The exception is contained by the
callback boundary, while an external `stop()` may synchronously join the same
transition. This is distinct from expected `OperationResult` failures, which
leave the session usable and produce no SDK log.
