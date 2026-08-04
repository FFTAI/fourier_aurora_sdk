# Fourier Aurora SDK

Fourier Aurora SDK 2.0.1 is the internal-preview C++ and Python client for
single-robot Aurora control over FourierDDS.

- Canonical C++ API under `fourier::aurora_sdk`
- Python package `fourier_aurora_client` backed by pybind11
- Restartable, process-wide high-level client
- Selective DDS endpoint creation and live match status
- Public C++ `FourierDdsTransport` facade for direct transport use

Generated DDS message classes remain in the separately managed message module.
The high-level client API uses SDK-owned state, command, result, and exception
types.

## Lifecycle

`AuroraClient::getInstance()` only returns the stable singleton. It does not
start DDS. The default domain is `123`. Lease-capable configurations require a
deployment-assigned nonzero `source_id`; an omitted Source Name becomes
`aurora_sdk_{source_id}`.

Configuration and callback overrides may be changed only while `Stopped`.
`start()` creates one transport session from a frozen configuration. `stop()`
synchronously drains admitted operations/callbacks, destroys transport, clears
session caches, and leaves the singleton available for reconfiguration or
restart.

```cpp
using namespace fourier::aurora_sdk;

auto& client = AuroraClient::getInstance();
client.setDomainId(123);
auto options = client.config();  // Returns a copy.
options.source_id = 42;          // Stable module identity assigned by deployment.
options.enabled_endpoints = {
    Endpoint::AuroraStateSubscriber,
    Endpoint::FsmService,
};
client.configure(options);       // Replaces the complete option set.

client.start();
if (client.waitForEndpoints(std::chrono::seconds(5))) {
    AuroraState state;
    const OperationResult result = client.getAuroraState(state);
    if (!result.success()) {
        // The caller decides whether and how to report the result.
    }
}
client.stop();
```

## Control lease

Motion, control-group, and temporary motor-configuration commands are published
only while the client owns a lease covering every targeted resource.
Registration is explicit, renewal is automatic, and replacement is atomic: a
rejected replacement leaves the old lease usable until its deadline.

```cpp
auto lease = client.registerLease({
    AuroraResource::MotionCommand,
    AuroraResource::LeftLeg,
});
if (lease.usable()) {
    client.publishMotionCommand(command);
}
lease = client.replaceLease({AuroraResource::RightLeg});
client.releaseLease();
```

The SDK writes no lease state to disk. A restarted process registers a fresh
lease; if its former lease still exists, wait for the short expiry and retry.
Deployment owners should maintain a reviewed
[Source ID registry](docs/source_id_registry.md).

Python exposes the same lifecycle and snake-case operation names:

```python
from fourier_aurora_client import AuroraClient, Endpoint

client = AuroraClient.get_instance()
options = client.config()
options.enabled_endpoints = [
    Endpoint.AURORA_STATE_SUBSCRIBER,
    Endpoint.FSM_SERVICE,
]
client.configure(options)

client.start()
if client.wait_for_endpoints(5.0):
    result, state = client.get_aurora_state()
client.stop()
```

Always call `stop()` explicitly before normal program shutdown. Omitting it
leaves cleanup to process destruction; an admitted service request or blocked
C++ callback can delay or hang shutdown. C++ callbacks execute on a DDS
callback thread, must finish in bounded time, and must not call `stop()`
directly. Python callback registration is intentionally unavailable in stage
one.

`Running` means only that local transport is active. Use endpoint match status
or `waitForEndpoints()` to determine discovery; neither condition is robot
readiness or command acknowledgement.

## Results and Commands

All getters, setters, publishes, and RPCs return `OperationResult`. C++ getters
use an output parameter and leave it unchanged on failure. Python getters
return `(OperationResult, value)`.

- `0`: success
- `100-199`: SDK client results
- `200-299`: Aurora-owned results
- `300-339`: typed SDK lease and sub-controller results

Aurora RPC codes and messages pass through unchanged, including temporary
legacy FSM codes `1` and `2`. Expected nonzero results are not logged by the
SDK. API misuse throws a small typed exception hierarchy; fatal connection
failures are logged and raise `ConnectionError`.

Values use SI units unless explicitly stated otherwise. Motion linear/angular
velocity and read-only Cartesian state use the robot base frame; motion
position uses the world frame. Local publish success means DDS accepted the
sample, not that Aurora accepted or executed the command. See
[command contract](docs/command_contract.md) and
[result codes](docs/result_codes.md).

## Dependencies

Stage one is tied to package-enforced dependency revisions:

- `fourier_dds 1.2.0-1`, source commit
  `d209ad3c339540ae737ae8f24722186c1631bba2`
- `fourier-dds-msgs 0.1.0-1`, source commit
  `18a24585984e04be401cf1acc0151d5b5e95b2cd`
- Matching FastDDS/FastCDR runtime supplied by those packages
- pybind11 when Python is enabled

Pass one dependency prefix to CMake:

```bash
-DFOURIER_AURORA_SDK_DDS_PREFIX=/opt/fftai
```

The validated development environment is
`docker.fftaicorp.com/base/aurora_medium:latest`. See
[dependency discovery](docs/dependency_discovery.md).

## Build

Native Release builds enable the DDS transport, Python binding, diagnostic
tools, and C++ recipes by default. Tests remain off:

```bash
cmake -S . -B build \
  -DCMAKE_BUILD_TYPE=Release \
  -DFOURIER_AURORA_SDK_DDS_PREFIX=/opt/fftai
cmake --build build -j4
```

Independent controls are:

```text
FOURIER_AURORA_SDK_BUILD_PYTHON
FOURIER_AURORA_SDK_BUILD_CLIENT_TOOLS
FOURIER_AURORA_SDK_BUILD_EXAMPLES
FOURIER_AURORA_SDK_BUILD_TESTS
FOURIER_AURORA_SDK_ENABLE_DDS_TRANSPORT
```

Build scripts and verification profiles are described in
[testing](docs/testing.md).

## Tools and Recipes

Native packages install diagnostic executables under
`/opt/fftai/fourier_aurora_sdk/bin/client/`. Their required positional
arguments, endpoint bundles, options, monitor behavior, and process exit codes
are documented in the [diagnostic client reference](docs/client_tools.md).

Standalone GR3 recipes are under:

- [C++ recipes](examples/gr3/cpp)
- [Python recipes](examples/gr3/python)

They demonstrate one-snapshot information reads, a bounded forward walk, and a
bounded arm swing workflow. Motion recipes require operator confirmation and
must be reviewed on the named robot before use. GR3 Mini recipes remain a
required pre-release TODO; robot-specific examples will not share source even
when workflows currently match.

Direct C++ transport users should read the dedicated
[FourierDDS transport reference](docs/fourierdds_transport.md). The high-level
singleton remains the recommended robot-control API.

## Release Scope

Stage one validates Linux amd64 locally: C++ Debian and CPython 3.10 wheel.
Linux arm64 artifacts are deferred to CI integration. The eventual public
matrix is Linux-only C++ Debian for amd64/arm64 and standard-GIL CPython
3.9-3.13 wheels for amd64/arm64.

Public developer tutorials, robot capability tables, governance,
`SECURITY.md`, and complete third-party attribution are public-release TODOs.
This repository currently documents the interface and examples needed by
internal developers. See the [release checklist](docs/release_checklist.md)
for the complete deferred matrix and publication gates.

## License

[Apache License 2.0](LICENSE)
