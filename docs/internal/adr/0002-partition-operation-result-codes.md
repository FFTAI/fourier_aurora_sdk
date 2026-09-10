---
status: accepted
---

# Partition operation result codes by owner

SDK operations use decimal `uint32_t` result codes with `0` for success,
`100-199` for client outcomes, and `200-299` for Aurora replies. Aurora values
and messages pass through unchanged. This keeps logs and integrations readable
while preventing client and robot failures from assigning the same public code;
the published registry becomes a compatibility contract.

Until Aurora completes its result-code migration, legacy FSM failure replies
`1` and `2` are passed through unchanged as a documented compatibility
exception. The SDK does not remap them into either owned range. They remain a
public-release dependency, not permanent registry allocations.

After argument validation, client-side failures use the fixed precedence
disabled endpoint, non-running client, unmatched endpoint, then missing state
or transport outcome. This ordering and unchanged-output-on-failure behavior
are part of the result contract. Any nonnegative Aurora reply is returned with
its exact message; unexpected values found in real integration block
compatibility review rather than being silently rewritten.
