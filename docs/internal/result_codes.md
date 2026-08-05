# Operation Result Codes

SDK getters, setters, publishes, and RPCs return an `OperationResult` containing
a `uint32_t` code and message. Code `0` always means success. RPC responses
carry Aurora's response code and message unchanged.

For one-way command publishes, code `0` means DDS publication succeeded. It
does not confirm that Aurora accepted or executed the command. Aurora rejection
is reported asynchronously through `ErrorCodes` and may not correlate to a
specific publication.

The SDK does not log nonzero operation results. Callers own reporting and retry
policy. Fatal SDK failures are outside this registry and are logged before an
exception is raised.

After structural argument validation, operation preconditions are evaluated in
this order:

1. required endpoint disabled;
2. client not running;
3. required endpoint unmatched;
4. state not received or the attempted transport outcome.

This makes multi-failure behavior deterministic. Subscription getters do not
return an old cache as a successful live read while the endpoint is unmatched,
and C++ output parameters remain unchanged on every failure. After rematch, a
latest-snapshot getter may return the retained sample with its original
timestamp; `waitForFsmState()` requires a sample from the current match
generation.

API misuse also remains outside this registry and throws a specific exception
without SDK logging. Code `106` represents an operation-local transport failure
where the client remains usable; a transport failure that makes continued
operation unsafe is fatal.

## Ranges

| Range | Owner |
|---:|---|
| `0` | Success |
| `100-199` | Aurora SDK client |
| `200-299` | Aurora service replies |
| `300-319` | SDK lease lifecycle and authority mapping |
| `320-339` | SDK sub-controller service mapping |

Codes outside these ranges are reserved. The SDK passes Aurora result values
and messages through unchanged. During the Aurora migration, legacy FSM
failure codes `1` and `2` are the only accepted temporary exception; they are
not client codes and are not remapped.

## Client Codes

| Code | Name | Meaning |
|---:|---|---|
| `0` | `SUCCESS` | Operation succeeded |
| `100` | `CLIENT_ERROR` | Unclassified client failure |
| `101` | `CLIENT_NOT_RUNNING` | Client is not running |
| `102` | `ENDPOINT_DISABLED` | Required endpoint is not configured |
| `103` | `ENDPOINT_UNMATCHED` | Required remote endpoint is not matched |
| `104` | `PUBLISH_FAILED` | DDS rejected the publication |
| `105` | `SERVICE_TIMEOUT` | No service response before timeout |
| `106` | `TRANSPORT_ERROR` | Other transport failure |
| `107` | `INVALID_AURORA_RESPONSE` | Malformed or invalid Aurora reply |
| `108` | `STATE_NOT_RECEIVED` | No state sample has been received yet |

## Lease Codes

| Code | Name | Meaning |
|---:|---|---|
| `300` | `LEASE_INVALID_REQUEST` | Aurora rejected the lease request structure |
| `301` | `LEASE_UNKNOWN_RESOURCE` | A requested resource is not in Aurora's catalog |
| `302` | `LEASE_PRIORITY_NOT_ALLOWED` | The source may not use the requested priority |
| `303` | `LEASE_RESOURCE_BUSY` | Another lease owns at least one requested resource |
| `304` | `LEASE_NOT_FOUND` | Aurora does not recognize the lease token |
| `305` | `LEASE_EXPIRED` | The lease period has ended |
| `306` | `LEASE_OPERATION_ID_CONFLICT` | An operation ID was reused with different content |
| `307` | `LEASE_AUTHORITY_ERROR` | Aurora reported an internal authority failure |
| `308` | `LEASE_SOURCE_ALREADY_ACTIVE` | This source already owns a module lease |
| `309` | `LEASE_AUTHORITY_BUSY` | Aurora cannot safely retain another transition |
| `310` | `LEASE_NOT_ACTIVE` | The client has no current usable lease |
| `311` | `LEASE_RESOURCE_NOT_COVERED` | The current lease does not cover the complete command |
| `312` | `LEASE_OPERATION_IN_PROGRESS` | Register, replace, or release is in progress |
| `313` | `LEASE_LOST` | The SDK has permanently stopped using the lease |
| `314` | `LEASE_OPERATION_TIMEOUT` | The public lease operation deadline elapsed |

## Sub-controller Codes

| Code | Name | Meaning |
|---:|---|---|
| `320` | `SUB_CONTROLLER_NO_ACTIVE_UNICONTROLLER` | The active state has no single running universal controller |
| `321` | `SUB_CONTROLLER_STATE_SWITCH_BUSY` | Aurora is switching states |
| `322` | `SUB_CONTROLLER_INVALID_REQUEST` | The complete controller request is invalid |
| `323` | `SUB_CONTROLLER_INTERNAL_ERROR` | Aurora could not process the service request |

## Aurora Codes

Aurora owns codes `200-299`. The current Aurora FSM service must be migrated
from legacy values `1` and `2` and must populate its response message before
the public SDK release. Until that migration lands, the SDK returns those two
legacy values and their messages exactly. A negative wire result is malformed
and returns client code `107`. Any other nonnegative out-of-range Aurora value
is also returned unchanged, but it blocks release verification pending
compatibility review rather than being silently remapped.
