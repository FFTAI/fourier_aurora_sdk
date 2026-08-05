---
status: accepted
---

# Keep authority snapshots advisory

The SDK exposes Aurora's nine authority-resource entries through typed
`AuroraState`. It adds no separate availability cache or resource-wait API, and
registration does not depend on receiving an AuroraState sample.

## Consequences

Applications may use the snapshot for UI, diagnostics, and retry hints but not
as authority. Delayed availability or holder display names never grant or
revoke the client-owned lease. Management outcomes and the local Conservative
Client Deadline determine usability and authority loss, including after Aurora
restart or takeover.
