---
status: accepted
---

# Partition operation result codes by owner

SDK operations use decimal `uint32_t` result codes with `0` for success,
`100-199` for client outcomes, and `200-299` for Aurora replies. Aurora values
and messages pass through unchanged. This keeps logs and integrations readable
while preventing client and robot failures from assigning the same public code;
the published registry becomes a compatibility contract.
