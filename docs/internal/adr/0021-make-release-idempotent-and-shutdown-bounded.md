---
status: accepted
---

# Make release idempotent and shutdown bounded

`releaseLease()` enters `Releasing` and blocks protected commands before its
first transport action. It exactly retries for at most 500 ms and never resumes
the old token after release begins.

## Consequences

Wire `OK`, `LEASE_NOT_FOUND`, and `LEASE_EXPIRED` all satisfy the public release
postcondition and become successful `LeaseResult` outcomes in `Unregistered`.
Calling release while already Unregistered is a local idempotent success. An
uncertain timeout becomes `Lost` and permits only bounded cleanup.

`AuroraClient::stop()` and the no-throw destructor attempt the same bounded
best-effort release, then discard all token, sequence, deadline, and retry
state. Shutdown is not blocked indefinitely by DDS or authority availability;
any unreleased Aurora grant ends by ordinary expiry. Restart always begins
Unregistered and restores nothing. Once stop completes, polling while Stopped
returns an Unregistered lease snapshot with configured Source Name and zero
token/resources/durations; Lost is reserved for uncertainty inside a running
session.
