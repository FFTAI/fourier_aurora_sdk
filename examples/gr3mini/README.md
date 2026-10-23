# GR3-Mini onboard examples

[中文](README_CN.md)

For GR3-Mini `v3_0_0`, SDK `1.0.1`, and a matching robot environment. Develop on the robot motherboard in its supplied runtime environment. Only this public SDK repository is required, without Aurora or AMC sources or a policy training environment. For container deliveries, perform all checks and runs inside the same supplied container.

## Check the existing environment first

```bash
uname -m
python3 --version
python3 -m pip show fourier-aurora-client
python3 -c 'from fourier_aurora_client import AuroraClient; print("SDK import OK")'
dpkg-query -W fourier_dds fourier-dds-msgs fourier-aurora-sdk
aurora-client --help
```

Use standard-GIL CPython 3.10–3.13. Native dependencies are `fourier_dds 1.2.0-1` and `fourier-dds-msgs 0.2.0-1`. CPU architecture, Python ABI, and wheel must match. The wheel contains neither DDS native libraries nor the CLI, which is installed by the SDK Debian package. Reuse a working installation. Check with the same Python interpreter used to run the script, rather than installing into a different environment.

For missing components, obtain matching packages from the robot installation media or delivery support, verify checksums, and follow [dependencies and installation](../../docs/overview.md#dependencies). Cloning this SDK does not provide the robot runtime or DDS dependencies. Do not substitute incompatible FastDDS or message libraries. A new virtual environment isolates system Python packages and needs a matching SDK wheel; system native libraries remain required. The joystick script also needs the supplied PyYAML, rich, pygame, and device permissions; these are not SDK wheel dependencies.

## Python examples

Run from the SDK repository root. The default action is read-only, without leases or state changes:

```bash
python3 examples/gr3mini/python/gr3mini_client.py --help
python3 examples/gr3mini/python/gr3mini_client.py read --domain 123
```

Match the robot domain and namespace; add `--namespace NAME` if configured. The example sets its identity internally. `--source` is normally unnecessary; use distinct names only for concurrent instances.

Prepare the robot using the delivered operating procedure and confirm the matching hardware and target state before motion. Exit joystick control first to avoid lease conflicts:

```bash
# Already in 110302 or 110311; requires typing YES
python3 examples/gr3mini/python/gr3mini_client.py walk --domain 123
# Already in 201; small head motion, requires typing YES
python3 examples/gr3mini/python/gr3mini_client.py head --domain 123
```

The script checks state, feedback freshness, and leases, then attempts to stop motion and release authority on exit. It does not prepare the state, implement an emergency stop, or provide recovery control. Read the [Python source](python/gr3mini_client.py) before running motion commands.

## C++ read-only example

Requires a C++17 compiler, CMake >= 3.16, and installed SDK development files. Adjust domain and connection settings in [cpp/read_state.cpp](cpp/read_state.cpp) to match deployment, then build:

```bash
cmake -S examples/gr3mini -B build-gr3mini \
  -DCMAKE_PREFIX_PATH='/opt/fftai/fourier_aurora_sdk;/opt/fftai'
cmake --build build-gr3mini --parallel 2
./build-gr3mini/gr3mini_read_state
```

Replace paths for a custom installation prefix. If CMake cannot find the package, check the SDK DEB and prefix. For import/runtime shared-library errors, check native libraries and system linker configuration; see the [build guide](../../docs/overview.md). That guide also covers building the SDK itself, without rebuilding the robot control software.

Python covers state reading, walking, and head control; C++ demonstrates the read-only lifecycle. Keep documentation-center download copies aligned with these sources. See the [Python API](../../docs/python_api.md) and [C++ API](../../docs/cpp_api.md) for full interfaces. These examples have not been validated on hardware in this change.
