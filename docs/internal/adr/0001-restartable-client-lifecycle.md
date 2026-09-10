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

Normal stop has no timeout and returns only after admitted operations and
callbacks quiesce. A private coordinator handles the exceptional case where a
fatal operation originates in a user callback and therefore cannot wait on its
own callback thread. That path converges the client to `Stopped` but does not
create a public nonblocking stop operation.

FourierDDS owns lower-level deletion diagnostics that its cleanup API does not
surface. The SDK logs and throws only teardown failures it can observe; it does
not claim to translate hidden transport warnings.
