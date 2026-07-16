# Testing

Tests are disabled in normal CMake, wheel, and Debian builds. GoogleTest and
pytest are test-only dependencies and are required only when
`FOURIER_AURORA_SDK_BUILD_TESTS=ON`.

Run a local profile from the repository root:

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
environments; enable it with `ASAN_OPTIONS=detect_leaks=1:halt_on_error=1` on a
compatible host.

The TSan profile uses `setarch -R` to reserve sanitizer shadow memory reliably.
Container security policies that prohibit the `personality` system call must be
adjusted before running that profile; the normal Debug, Release, and ASan+UBSan
profiles do not require this setting.

The profiles use mock transport by default. Set `ENABLE_DDS=ON` only in an
environment containing matching FourierDDS core and generated message packages.
Build directories default to `build_tests/<profile>` and can be relocated with
`BUILD_ROOT`.

The prescribed build image can run a profile without creating root-owned files:

```bash
docker run --rm \
  --user "$(id -u):$(id -g)" \
  -e HOME=/tmp \
  -v "$PWD:/workspace" \
  -w /workspace \
  docker.fftaicorp.com/base/aurora_medium:latest \
  ./scripts/test_debug.sh
```

CI performs a default Release build with tests disabled. These profiles are
manual local verification until dedicated test jobs are added later.
