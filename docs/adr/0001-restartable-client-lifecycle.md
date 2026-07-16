---
status: accepted
---

# Use a restartable Aurora client lifecycle

The process-scoped `AuroraClient` is created without transport resources and
uses explicit `start()` and synchronous `stop()` operations. Configuration and
callback overrides are allowed only while stopped, and stopping preserves the
client for later reconfiguration and restart; there is no terminal public
`close()`. This separates client identity from each DDS transport session,
prevents singleton self-destruction, and gives shutdown one deterministic
quiescence contract.

## Consequences

Client state is `Stopped`, `Starting`, `Running`, or `Stopping`; running means
only that local transport endpoints exist. Users must explicitly stop before
program teardown, while the destructor provides no-throw fallback cleanup.
