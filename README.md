# Fourier Aurora SDK

This repository contains the refactored Aurora SDK:

- C++ SDK under `include/fourier_aurora_sdk` and `src/`
- Python SDK package `fourier_aurora_client` backed by pybind11
- `fourierdds` transport for Aurora DDS topics and services

The public SDK API uses SDK-owned types. Generated DDS message types and raw `fourierdds` entities are internal implementation details.

## Dependencies

Stage 1 uses internal system packages. Install these before building:

- `fourierdds`
- generated `fourier_dds_msgs`
- FastDDS/FastCDR packages matching the installed message libraries
- pybind11, when building Python bindings

The validated internal build environment is:

```bash
docker.fftaicorp.com/base/aurora_medium:latest
```

## Build

C++ only:

```bash
cmake -S . -B build -DFOURIER_AURORA_SDK_BUILD_PYTHON=OFF
cmake --build build -j4
ctest --test-dir build --output-on-failure
```

C++ plus Python bindings, with DDS transport enabled:

```bash
cmake -S . -B build \
  -DCMAKE_PREFIX_PATH=/opt/fftai \
  -DFOURIER_AURORA_SDK_BUILD_PYTHON=ON \
  -DFOURIER_AURORA_SDK_ENABLE_DDS_TRANSPORT=ON
cmake --build build -j4
ctest --test-dir build --output-on-failure
```

Build a C++ SDK Debian package:

```bash
bash scripts/build_deb.sh
sudo dpkg -i build_deb/fourier-aurora-sdk_*.deb
```

The Debian package installs under:

```text
/opt/fftai/fourier_aurora_sdk
```

It also installs:

- `/etc/ld.so.conf.d/fourier_aurora_sdk.conf` for runtime library lookup

Compile and run the installed C++ smoke test directly with `g++`:

```bash
bash scripts/test_installed_cpp_sdk.sh
```

Build the Python wheel:

```bash
python3 -m build --wheel
pip install dist/fourier_aurora_client-*.whl
```

The Python wheel bundles the Aurora C++ SDK shared library next to the pybind module,
so installing the C++ SDK Debian package is not required for Python use. Stage 1 still
requires the native DDS stack to be installed on the system:

- generated `fourier_dds_msgs` libraries under `/opt/fftai/fourier_dds_msgs/lib`
- FastDDS/FastCDR libraries under `/usr/local/lib`
- `fourierdds` headers/libraries available at build time

The wheel is built with the DDS transport enabled and only builds the Python module
target, not the C++ examples.

Installed C++ usage examples are copied to:

```text
/opt/fftai/fourier_aurora_sdk/share/doc/fourier_aurora_sdk/examples/cpp/usage
```

When built with `FOURIER_AURORA_SDK_ENABLE_DDS_TRANSPORT=ON`, the example binaries are installed to:

```text
/opt/fftai/fourier_aurora_sdk/bin/fourier_aurora_sdk_examples
```

They accept optional `[domain_id] [user_id]` arguments:

```bash
/opt/fftai/fourier_aurora_sdk/bin/fourier_aurora_sdk_examples/get_info
/opt/fftai/fourier_aurora_sdk/bin/fourier_aurora_sdk_examples/set_state
/opt/fftai/fourier_aurora_sdk/bin/fourier_aurora_sdk_examples/set_motion
/opt/fftai/fourier_aurora_sdk/bin/fourier_aurora_sdk_examples/set_arm_swing
/opt/fftai/fourier_aurora_sdk/bin/fourier_aurora_sdk_examples/state_client
/opt/fftai/fourier_aurora_sdk/bin/fourier_aurora_sdk_examples/motion_client
/opt/fftai/fourier_aurora_sdk/bin/fourier_aurora_sdk_examples/control_group_client
```

Python usage examples are under:

```text
examples/python/usage
```

They mirror the minimal C++ usage examples:

```bash
python3 examples/python/usage/get_info.py 88 test
python3 examples/python/usage/set_state.py 88 test
python3 examples/python/usage/set_motion.py 88 test
python3 examples/python/usage/set_arm_swing.py 88 test
```

## Repository Structure

```text
cmake/                        CMake package config template
examples/                     Installed C++ smoke and usage examples
include/                      Public C++ SDK headers
src/                          C++ SDK implementation
python/bindings/              pybind11 bindings
python/fourier_aurora_client/ Python compatibility package
tests/                        Unit tests
docs/                         SDK architecture and dependency notes
scripts/                      Wheel build entry points
```

## Notes

- `AuroraClient` is a singleton. Call `close()` before creating a new instance with different options.
- Python keeps selected legacy snake_case methods, but `get_motion_state()` compatibility dict is intentionally removed. Use `getMotionState()`.
- `error_codes_pub` is exposed in C++ first; Python helper shape is deferred.

## License

[Apache 2.0](LICENSE) © Fourier
