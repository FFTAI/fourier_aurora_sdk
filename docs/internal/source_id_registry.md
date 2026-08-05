# Source ID Assignment

`source_id` identifies a deployed control module to Aurora. It is not allocated
by the SDK. Give each concurrently connected module a unique, nonzero
`uint32_t` value and keep that value stable when the same module restarts.

Maintain assignments in deployment configuration or a reviewed registry:

| Source ID | Stable module role | Owner | Emergency eligible | Notes |
|---:|---|---|:---:|---|
|  |  |  | no |  |

Two live modules using the same ID are indistinguishable to the lease authority
and can interfere with one another's lease lifecycle. Emergency eligibility
must also match Aurora's `LeaseConfig.emergency_source_ids`; enabling the SDK
opt-in alone does not grant that privilege.

`source_name` is optional display text. When omitted, the SDK sends
`aurora_sdk_{source_id}`. It does not participate in resource ownership or
priority decisions.
