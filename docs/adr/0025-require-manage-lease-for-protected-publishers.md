---
status: accepted
---

# Require ManageLease for protected publishers

`Endpoint::ManageLeaseService` is configurable through the existing enabled-
endpoint model and is part of readiness reporting. Every configuration that
enables a protected command publisher must also enable ManageLease; invalid
combinations fail `start()` before transport creation.

## Consequences

Read-only and lease-exempt-service clients may omit lease transport, and a
lease-only coordinator may enable ManageLease alone. Default command-capable
configuration enables it. If matching is lost after registration, existing
authority remains usable only until its Conservative Client Deadline.

All high-level protected paths use the client-owned lease gate and public
command types cannot inject raw LeaseInfo. Raw DDS participants remain outside
the SDK and may transmit samples, so Aurora independently rejects commands
without a valid token and remains the authority boundary.
