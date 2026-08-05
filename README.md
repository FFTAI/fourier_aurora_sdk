# Fourier Aurora SDK

[中文](README_CN.md)

Fourier Aurora SDK 2.0.1 is the C++17 and Python client library for reading
Aurora robot state and sending commands over FourierDDS. Applications use
SDK-owned types; generated DDS messages stay behind the SDK boundary.

## Choose a package

- Install the `fourier-aurora-sdk` Debian package for C++ headers, the shared
  library, CMake package files, diagnostic clients, and GR3 examples.
- Install the `fourier-aurora-client` wheel for the Python package
  `fourier_aurora_client`.
- Build from source when developing the SDK or producing packages for a
  deployment. See the [SDK overview and build guide](docs/overview.md).

The matching `fourier_dds` and `fourier-dds-msgs` runtime packages must be
installed first. The supported revisions are recorded in `REVISIONS.yaml`.

## Python quick start: read robot state

Install the wheel supplied for your platform, then create only the endpoints
your program uses:

```bash
python3 -m pip install fourier_aurora_client-2.0.1-*.whl
```

```python
from fourier_aurora_client import AuroraClient, Endpoint

client = AuroraClient.get_instance()
options = client.config()
options.enabled_endpoints = [Endpoint.AURORA_STATE_SUBSCRIBER]
client.configure(options)

try:
    client.start()
    if not client.wait_for_endpoints(5.0):
        raise RuntimeError("Aurora state endpoint did not match")

    result, state = client.get_aurora_state()
    if not result.success():
        raise RuntimeError(f"[{result.code}] {result.message}")
    print(state.robot_info.robot_type, state.current_state.name)
finally:
    client.stop()
```

Python timeout values are seconds. `Running` means the local transport has
started; use `wait_for_endpoints()` to check DDS discovery. A matched endpoint
still does not guarantee that the robot is ready to execute a command.

## C++ quick start: read robot state

After installing the Debian package, link the exported CMake target:

```cmake
find_package(fourier_aurora_sdk 2.0 CONFIG REQUIRED)
target_link_libraries(my_app PRIVATE fourier_aurora_sdk::fourier_aurora_sdk)
target_compile_features(my_app PRIVATE cxx_std_17)
```

```cpp
#include <chrono>
#include <iostream>
#include "fourier_aurora_sdk/aurora_client.hpp"

namespace sdk = fourier::aurora_sdk;

int main() {
    auto& client = sdk::AuroraClient::getInstance();
    auto options = client.config();
    options.enabled_endpoints = {sdk::Endpoint::AuroraStateSubscriber};
    client.configure(options);

    try {
        client.start();
        if (!client.waitForEndpoints(std::chrono::seconds(5))) {
            client.stop();
            return 1;
        }
        sdk::AuroraState state;
        const auto result = client.getAuroraState(state);
        if (result.success()) {
            std::cout << state.robot_info.robot_type << " "
                      << state.current_state.name << "\n";
        }
        client.stop();
        return result.success() ? 0 : 1;
    } catch (...) {
        try { client.stop(); } catch (...) {}
        throw;
    }
}
```

C++ timeout values use `std::chrono`. Call `stop()` explicitly during normal
shutdown.

## Sending a protected command

Motion, control-group, and temporary motor-configuration publishers require a
control lease. A deployment owner assigns a nonzero module `source_id`; do not
invent an ID in a multi-client deployment.

```cpp
options.source_id = 42;
options.enabled_endpoints = {
    sdk::Endpoint::ManageLeaseService,
    sdk::Endpoint::MotionCommandPublisher,
};
client.configure(options);
client.start();
client.waitForEndpoints(std::chrono::seconds(5));

const auto lease = client.registerLease({sdk::AuroraResource::MotionCommand});
if (lease.usable()) {
    const auto result = client.setVelocity(0.1, 0.0, 0.0, 1.0);
}
client.releaseLease();
client.stop();
```

The SDK automatically renews an active lease. `replaceLease()` atomically asks
for a new complete resource set; a definitive rejection preserves the old
lease while it remains valid. Lease state is memory-only and is not recovered
after process restart.

Local publish success only means DDS accepted the sample. Observe Aurora state
and error telemetry to determine the robot outcome. Keep the robot supported,
clear the work area, and test control changes at conservative limits.

## Documentation

- [SDK overview, architecture, dependencies, and build guide](docs/overview.md)
- [C++ API reference](docs/cpp_api.md)
- [Python API reference](docs/python_api.md)
- [Examples and diagnostic clients](docs/examples_and_clients.md)
- [中文文档入口](README_CN.md)

Design records, implementation contracts, test profiles, and release work are
kept in the source tree under `docs/internal/`. They are maintainer material,
not a supported application API and are not installed with binary packages.

## License

[Apache License 2.0](LICENSE)
