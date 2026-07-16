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

## Aurora Codes

Aurora owns codes `200-299`. The current Aurora FSM service must be migrated
from legacy values `1` and `2` and must populate its response message before
the public SDK release. Until that migration lands, the SDK returns those two
legacy values and their messages exactly. A negative wire result is malformed
and returns client code `107`. Any other nonnegative out-of-range Aurora value
is also returned unchanged, but it blocks release verification pending
compatibility review rather than being silently remapped.
