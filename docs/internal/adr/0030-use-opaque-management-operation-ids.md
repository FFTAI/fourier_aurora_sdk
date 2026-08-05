---
status: accepted
---

# Use opaque management Operation IDs

For Stage 1, the SDK allocates a fresh nonzero 64-bit Operation ID for every
logical register, renew, release, or replace operation. Exact retries
reuse the same ID and canonical request content. The ID is internal to native
C++, memory-only, and never exposed through the public C++ or Python API.

The allocator starts at `1` for each random per-run Client Instance ID,
increments once per new logical operation, and never wraps. This counter is only
an implementation of uniqueness: Aurora gives its numeric order no protocol
meaning and performs no stale-sequence check. Exhaustion is a fatal local SDK
condition; restart creates a new Client Instance ID and restarts at `1`.

The SDK maintains at most one logical management operation in flight, including
serialization of automatic renewal against replacement and release. It does
not compare Operation IDs, maintain an acquisition counter, or maintain a
per-lease action counter.

## Consequences

`ManageLease_Request.sequence_id` is renamed to `operation_id`. Each relevant
active or terminal Lease Period retains only its latest replayable response.
Aurora replays that response for an exact ID/content retry and returns
`OPERATION_ID_CONFLICT` when retained content differs. Public SDK mapping uses
`LEASE_OPERATION_ID_CONFLICT`.

Once the SDK has received one response and started a newer operation, it never
retries the older operation. Rejected registration creates no replay record and
may be reevaluated. Aurora requires no general response-cache capacity setting.

Client restart creates a fresh Client Instance ID and fresh Operation IDs; no
identity is persisted. Raw DDS or ROS 2 management clients must generate a
unique Operation ID for each logical operation and reuse it for retries.

This supersedes the Acquisition Sequence and Lease-Action Sequence decisions
for Stage 1. A future versioned protocol stage will implement monotonic
management sequences with high-water ordering and stale-operation rejection.
