# Testing

Tests are disabled in normal CMake, wheel, and Debian builds. Native tests are
self-contained CTest executables and add no external C++ test framework.
Python tests use pytest only when `FOURIER_AURORA_SDK_BUILD_TESTS=ON`.

All current stage-one compilation and testing must run inside the existing
`aurora_medium_latest_container`, created from
`docker.fftaicorp.com/base/aurora_medium:latest`. The SDK is mounted at
`/workspace/fourier_aurora_sdk`. Host results are diagnostic only.

Invoke scripts as the host user so build output stays under container `/tmp`
and does not create root-owned source files:

```bash
docker exec \
  --user "$(id -u):$(id -g)" \
  --env HOME=/tmp \
  --workdir /workspace/fourier_aurora_sdk \
  aurora_medium_latest_container \
  ./scripts/test_debug.sh
```

## Fixed Profiles

Run each profile directly inside the container:

```bash
./scripts/test_debug.sh
./scripts/test_release.sh
./scripts/test_asan_ubsan.sh
./scripts/test_tsan.sh
```

Debug and Release build the DDS-independent C++ tests and Python binding, then
run CTest and pytest. ASan+UBSan and TSan run only the native tests because
system Python and DDS libraries are not instrumented. Every run uses a fresh
temporary build directory and retains it for inspection.

The profile composition is fixed. Legacy `ENABLE_DDS`, `RUN_PYTHON_TESTS`, and
`BUILD_ROOT` overrides are rejected. `TEST_TMP_ROOT` may only relocate the
fresh temporary directories. ASan leak detection defaults off for container
compatibility. TSan uses `setarch -R`; if the container security policy blocks
that system call, report the profile as blocked rather than substituting a host
run or changing the sanitizer behavior.

## DDS Integration

The DDS profile requires the one staged prefix containing the exact packages
listed in `REVISIONS.yaml`:

```bash
FOURIER_AURORA_SDK_DDS_PREFIX=/tmp/aurora_sdk_impl/prefix/opt/fftai \
  ./scripts/test_dds_transport.sh
```

It verifies CMake resolution from that prefix, builds the production transport,
and runs the real local state, command, service, callback-stop, timeout,
restart, and no-namespace loopback. Conflicting DDS environment defaults are
set deliberately to prove explicit SDK configuration wins.

## Package Verification

Before committing, build provisional clean-name artifacts from the dirty
implementation worktree:

```bash
INCLUDE_COMMIT_HASH=OFF \
FOURIER_AURORA_SDK_DDS_PREFIX=/tmp/aurora_sdk_impl/prefix/opt/fftai \
  ./scripts/build_deb.sh

INCLUDE_COMMIT_HASH=OFF \
FOURIER_AURORA_SDK_DDS_PREFIX=/tmp/aurora_sdk_impl/prefix/opt/fftai \
  ./scripts/build_wheel_x86_64.sh
```

The wheel build requires CPython 3.10, pybind11 2.13.6 or newer,
`scikit-build-core>=0.9.10`, `build`, and `venv` in the container. Both build
scripts create unique external build and artifact directories and print the
resulting artifact path. Install these tools in the active interpreter, use an
activated virtual environment, or expose a read-only target directory through
`PYTHONPATH`; the build does not download dependencies implicitly.

Verify the printed paths:

```bash
FOURIER_AURORA_SDK_DDS_PREFIX=/tmp/aurora_sdk_impl/prefix/opt/fftai \
  ./scripts/check_deb_package.sh /tmp/path/fourier-aurora-sdk_2.0.0_amd64.deb

./scripts/check_wheel_package.sh \
  /tmp/path/fourier_aurora_client-2.0.0-cp310-cp310-linux_x86_64.whl

FOURIER_AURORA_SDK_DDS_PREFIX=/tmp/aurora_sdk_impl/prefix/opt/fftai \
  ./scripts/test_python_wheel.sh \
  /tmp/path/fourier_aurora_client-2.0.0-cp310-cp310-linux_x86_64.whl
```

The Debian gate checks exact package dependencies and contents, standalone
compilation of every installed public header, an isolated exported-CMake
consumer, ELF `RUNPATH`, staged native dependency resolution, and an installed
C++ DDS state/RPC/motion loopback. The wheel gates check its mutually exclusive
manifest, full license, metadata, ELF linkage, fresh virtual-environment import,
isolated loader resolution, and the equivalent installed-Python loopback.

After the final source commits, rerun builds with the default
`INCLUDE_COMMIT_HASH=ON`. Hash-on mode rejects any tracked, staged, or untracked
worktree content and embeds exactly the current eight-character `HEAD` in both
artifact names. `INCLUDE_COMMIT_HASH=OFF` remains available for eventual clean
public names, subject to `release_checklist.md`.

## Coverage Limits

Current local package evidence covers Linux amd64 C++ Debian and CPython 3.10.
Required arm64 artifacts are deferred to CI and require an approved arm64
execution environment plus architecture-matched dependency packages. Hardware
and real Aurora-runtime recipe validation are separate release records; local
DDS loopback does not claim robot readiness or command execution.
