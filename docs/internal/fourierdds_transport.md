# Direct FourierDDS Transport

`fourier::aurora_sdk::FourierDdsTransport` is the public C++ facade for direct
use of the stage-one FourierDDS transport. Most robot applications should use
the higher-level restartable `AuroraClient` singleton instead.

## Lifetime

Construction immediately creates one configured transport session:

```cpp
fourier::aurora_sdk::ConnectionOptions options;
options.enabled_endpoints = {
    fourier::aurora_sdk::Endpoint::AuroraStateSubscriber,
    fourier::aurora_sdk::Endpoint::FsmService,
};

fourier::aurora_sdk::FourierDdsTransport transport(123, options);
// Inspect, publish, or request.
transport.stop();
```

The facade is noncopyable and nonmovable. `stop()` is synchronous, idempotent,
and terminal for that facade; post-stop operations return
`CLIENT_NOT_RUNNING`. To create another session, construct another facade.
The destructor performs no-throw fallback cleanup, but explicit `stop()` is
the required normal shutdown path.

`ConnectionOptions::service_timeout_ms` bounds FSM service requests. An
admitted request or callback can delay synchronous stop until it returns, so
applications should choose a finite service timeout and bounded callback work.

## Callbacks

Handlers may be installed or replaced while the direct facade is active.
Registration affects future samples and does not replay old data. Handlers run
inline on a FourierDDS callback thread and must return in bounded time. They
must not call `stop()` directly; callback-origin stop raises
`InvalidStateError`. Signal handlers likewise must not call SDK methods.

The facade catches and logs handler exceptions at the asynchronous boundary so
they do not terminate the process or stop later delivery.

## Commands and Results

Publish and request methods return `OperationResult`. Expected nonzero results
are not logged by the SDK. Disabled endpoints, stopped sessions, and unmatched
endpoints follow the same result-code contract as the high-level client.

Unlike `AuroraClient`, the direct facade does not stamp command headers or keep
last-command caches. The caller owns `MessageHeader::send_time`; the exact
supplied timestamp is forwarded to the wire.

## Namespace and Matching

The facade applies the explicit namespace and compatibility settings in
`ConnectionOptions`. It does not perform BMS/file lookup. Only enabled
endpoints are created. `endpointMatchStatus()` reports enabled and live match
state, while endpoint-specific convenience methods return false after stop.

Running transport and endpoint matching do not imply that Aurora is healthy or
ready to execute commands.

## Session Scope

The one-robot-per-process decision applies to the high-level `AuroraClient`.
Direct facade users may explicitly own additional sessions, but then assume
responsibility for process lifetime, DDS resource use, and Aurora command
arbitration. This facade is not a multi-robot orchestration API.
