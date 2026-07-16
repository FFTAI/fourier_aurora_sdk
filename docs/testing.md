# Testing

Tests are disabled in normal CMake, wheel, and Debian builds. Native tests are
self-contained CTest executables and add no external C++ test framework.
Python tests use pytest only when `FOURIER_AURORA_SDK_BUILD_TESTS=ON`.

All current stage-one amd64 build and test profiles must run inside the existing
`aurora_medium_latest_container`, created from
`docker.fftaicorp.com/base/aurora_medium:latest`. The SDK is mounted at
`/workspace/fourier_aurora_sdk`. From that directory in the container, run:

```bash
./scripts/test_debug.sh
./scripts/test_release.sh
./scripts/test_asan_ubsan.sh
./scripts/test_tsan.sh
```

Debug and Release run C++ tests and, when present, Python tests. ASan+UBSan and
TSan run native tests by default because sanitizer use through a system Python
or uninstrumented DDS libraries can produce unreliable reports. Set
`RUN_PYTHON_TESTS=ON` explicitly when using a compatible instrumented Python.
ASan leak detection defaults off because it is incompatible with ptraced build
environments; enable it with `ASAN_OPTIONS=detect_leaks=1:halt_on_error=1` only
in a compatible configuration of the required container.

The TSan profile uses `setarch -R` to reserve sanitizer shadow memory reliably.
If the container security policy prohibits the `personality` system call, stop
and report the blocked test. Do not substitute a host run or silently change
the test environment.

The profiles use mock transport by default. Set `ENABLE_DDS=ON` only in an
environment containing matching FourierDDS core and generated message packages.
Build directories default to `build_tests/<profile>` and can be relocated with
`BUILD_ROOT`.

Invoke a profile in the mandatory named container without creating root-owned
files:

```bash
docker exec \
  --user "$(id -u):$(id -g)" \
  --env HOME=/tmp \
  --workdir /workspace/fourier_aurora_sdk \
  aurora_medium_latest_container \
  ./scripts/test_debug.sh
```

Host builds and host test results are diagnostic only and do not satisfy
implementation verification. A missing dependency or blocked test in the
mandatory container requires user direction before work continues. CI performs
a default Release build with tests disabled; these container profiles remain
manual verification until dedicated test jobs are added later.

Current local package evidence covers Linux amd64 C++ Debian and CPython 3.10
wheel artifacts. Required arm64 Debian and CPython 3.10 wheel builds are part of
the later CI integration and require an approved arm64-native or explicitly
approved cross-build environment. This amd64 profile must not be reported as
native arm64 validation.
