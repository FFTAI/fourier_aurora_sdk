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
PYTHON_BIN=python3.10 \
  ./scripts/build_wheel_x86_64.sh
```

The wheel build accepts standard-GIL CPython 3.10 through 3.13 and produces one
wheel for the selected `PYTHON_BIN`. It requires pybind11 2.13.6 or newer,
`scikit-build-core>=0.9.10`, `build`, and `venv` for that interpreter. Both build
scripts create unique external build and artifact directories and print the
resulting artifact path. Install these tools in the active interpreter, use an
activated virtual environment, or expose a read-only target directory through
`PYTHONPATH`; the build does not download dependencies implicitly.

Verify the printed paths:

```bash
FOURIER_AURORA_SDK_DDS_PREFIX=/tmp/aurora_sdk_impl/prefix/opt/fftai \
  ./scripts/check_deb_package.sh /tmp/path/fourier-aurora-sdk_1.0.1_amd64.deb

./scripts/check_wheel_package.sh \
  /tmp/path/fourier_aurora_client-1.0.1-cp310-cp310-linux_x86_64.whl

FOURIER_AURORA_SDK_DDS_PREFIX=/tmp/aurora_sdk_impl/prefix/opt/fftai \
PYTHON_BIN=python3.10 \
  ./scripts/test_python_wheel.sh \
  /tmp/path/fourier_aurora_client-1.0.1-cp310-cp310-linux_x86_64.whl
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

The amd64 CI recipe retains its separate CPython 3.10 through 3.13 wheels.
The ARM64 recipe builds the ABI provided by the selected image's target SDK
(`TARGET_PYTHON_VERSION`, Python 3.12 for the Thor Noble image). It rejects a
requested Python version that lacks a matching target SDK rather than using
host-only development headers. The host build interpreter must have the same
major/minor version as the target; Python discovery for compiled bindings uses
only the target development headers. No target interpreter is executed during
cross compilation.

Like Aurora, FCB enters through `ci/fcb_build.sh`: the shared helper normalizes
architecture, sysroot and parallelism; the module adapter maps the toolchain and
DDS prefix; `scripts/build_packages.sh` builds the packages; the helper collects
both DEB and wheel into `fcb-output`. The manifest only selects this entrypoint.
`FCB_TOOLCHAIN_FILE` takes precedence over the image's `CMAKE_TOOLCHAIN_FILE`.
For ARM, the default DDS prefix is `${FCB_SYSROOT}/opt/fftai`; for amd64 it is
`/opt/fftai`. An explicit `FOURIER_AURORA_SDK_DDS_PREFIX` overrides either default.

For the same entrypoint used by CI, run:

```bash
FCB_HELPER_DIR=/path/to/fcb/shell FCB_ARCH=arm64 FCB_JOBS=4 \
  FCB_SYSROOT="$SYSROOT" FCB_TOOLCHAIN_FILE="$CMAKE_TOOLCHAIN_FILE" \
  bash ci/fcb_build.sh
```

For standalone packaging, run
`ARCH=arm64 JOBS=4 bash scripts/build_packages.sh` inside the Thor cross image,
after staging the manifest-pinned fourierddsmsgs and fourierdds DEBs into
`SYSROOT`. The image must supply `CMAKE_TOOLCHAIN_FILE` and
`TARGET_PYTHON_VERSION`; the recipe uses `${SYSROOT}/opt/fftai` by default.
`ci/build_packages.sh` remains a compatibility wrapper for existing callers.
It generates an arm64 DEB and a `cp312-cp312-linux_aarch64` wheel for Thor.
Both scripts validate the package architecture against the actual ELF objects.
Use `INCLUDE_COMMIT_HASH=OFF` only for development iterations; final builds
require a clean commit as described above.

Validate runtime behavior on a native ARM64 Linux container matching the target
OS: extract the DDS dependencies and SDK DEB into isolated prefixes, then run
`test_installed_cpp_sdk.sh` and `test_python_wheel.sh` using those prefixes. The
wheel checker accepts both x86_64 and aarch64 and checks ELF machine, tags,
metadata, private-library loading, and exported content. The runtime tests
exercise state subscription, lease registration, FSM RPC, and motion publishing
against a local test peer. The peer supplies its own namespace and does not
query a robot's BMS or configuration. Hardware and real Aurora-runtime acceptance
remain separate; local DDS loopback does not establish robot readiness.
