#!/usr/bin/env bash
set -euo pipefail

project_root="$(cd "$(dirname "$0")/.." && pwd)"
include_commit="${INCLUDE_COMMIT_HASH:-OFF}"
dds_prefix="${FOURIER_AURORA_SDK_DDS_PREFIX:-}"
python_bin="${PYTHON_BIN:-python3}"

require_external_root() {
    local label="$1"
    local root
    root="$(realpath -m "$2")"
    if [[ "${root}" == "${project_root}" || "${root}" == "${project_root}/"* ]]; then
        echo "${label} must be outside the SDK source tree: ${root}" >&2
        exit 2
    fi
}

wheel_arch="${FOURIER_AURORA_SDK_WHEEL_ARCH:-x86_64}"
if [[ "${wheel_arch}" == x86_64 && "$(uname -m)" != "x86_64" ]]; then
    echo "This script builds the Linux amd64 wheel and requires an x86_64 host." >&2
    exit 2
fi
if [[ -z "${dds_prefix}" ]]; then
    echo "Set FOURIER_AURORA_SDK_DDS_PREFIX to the exact staged dependency prefix." >&2
    exit 2
fi
if [[ ! -d "${dds_prefix}" ]]; then
    echo "DDS dependency prefix does not exist: ${dds_prefix}" >&2
    exit 2
fi

case "${include_commit}" in
    ON)
        commit="$(${project_root}/scripts/resolve_commit_hash.sh)"
        build_tag="1g${commit}"
        ;;
    OFF)
        commit=""
        build_tag=""
        ;;
    *)
        echo "INCLUDE_COMMIT_HASH must be ON or OFF." >&2
        exit 2
        ;;
esac

if ! command -v "${python_bin}" >/dev/null 2>&1; then
    echo "Python interpreter is unavailable: ${python_bin}" >&2
    exit 2
fi
python_version="$("${python_bin}" -c 'import platform, sys; assert platform.python_implementation() == "CPython"; print(f"{sys.version_info.major}{sys.version_info.minor}")')"
case "${python_version}" in
    310|311|312|313) ;;
    *)
        echo "Wheel builds require standard-GIL CPython 3.10 through 3.13; found cp${python_version}." >&2
        exit 2
        ;;
esac
if [[ "$("${python_bin}" -c 'import sysconfig; print(sysconfig.get_config_var("Py_GIL_DISABLED") or 0)')" != "0" ]]; then
    echo "Free-threaded CPython is not supported: ${python_bin}" >&2
    exit 2
fi
python_tag="cp${python_version}"
if ! "${python_bin}" -c 'import build, pybind11, scikit_build_core' >/dev/null 2>&1; then
    echo "Install build, pybind11, and scikit-build-core for ${python_bin}." >&2
    exit 2
fi
version="$(tr -d '[:space:]' < "${project_root}/VERSION")"

build_root="${BUILD_ROOT:-/tmp}"
artifact_root="${ARTIFACT_ROOT:-/tmp}"
require_external_root BUILD_ROOT "${build_root}"
require_external_root ARTIFACT_ROOT "${artifact_root}"
mkdir -p "${build_root}" "${artifact_root}"
build_dir="$(mktemp -d "${build_root%/}/fourier-aurora-sdk-wheel-build.XXXXXX")"
artifact_dir="$(mktemp -d "${artifact_root%/}/fourier-aurora-sdk-wheel-artifacts.XXXXXX")"
export CMAKE_GENERATOR="Unix Makefiles"

build_args=(
    --wheel
    --no-isolation
    --skip-dependency-check
    --outdir "${artifact_dir}"
    --config-setting="build-dir=${build_dir}"
    --config-setting="cmake.build-type=Release"
    --config-setting="cmake.define.FOURIER_AURORA_SDK_DDS_PREFIX=${dds_prefix}"
    --config-setting="cmake.define.FOURIER_AURORA_SDK_ENABLE_DDS_TRANSPORT=ON"
    --config-setting="cmake.define.FOURIER_AURORA_SDK_BUILD_PYTHON=ON"
    --config-setting="cmake.define.FOURIER_AURORA_SDK_BUILD_TESTS=OFF"
    --config-setting="cmake.define.FOURIER_AURORA_SDK_BUILD_CLIENT_TOOLS=OFF"
    --config-setting="cmake.define.FOURIER_AURORA_SDK_BUILD_EXAMPLES=OFF"
)
if [[ "${include_commit}" == "ON" ]]; then
    build_args+=(--config-setting="wheel.build-tag=${build_tag}")
fi

if [[ "${wheel_arch}" == aarch64 ]]; then
    : "${SYSROOT:?ARM wheel requires SYSROOT}"
    : "${CMAKE_TOOLCHAIN_FILE:?ARM wheel requires CMAKE_TOOLCHAIN_FILE}"
    : "${TARGET_PYTHON_VERSION:?ARM wheel requires TARGET_PYTHON_VERSION}"
    if [[ "${python_version}" != "${TARGET_PYTHON_VERSION/./}" ]]; then
        echo "Host Python must match target Python ${TARGET_PYTHON_VERSION}." >&2
        exit 2
    fi
    target_include="${SYSROOT}/usr/include/python${TARGET_PYTHON_VERSION}"
    [[ -f "${target_include}/Python.h" ]] || { echo "Missing target Python headers: ${target_include}" >&2; exit 2; }
    target_suffix=".cpython-${python_version}-aarch64-linux-gnu.so"
    build_args+=(
        --config-setting="cmake.python-hints=false"
        --config-setting="cmake.toolchain-file=${CMAKE_TOOLCHAIN_FILE}"
        --config-setting="wheel.tags=${python_tag}-${python_tag}-linux_aarch64"
        --config-setting="cmake.define.Python_INCLUDE_DIR=${target_include}"
        --config-setting="cmake.define.FOURIER_AURORA_SDK_HOST_PYTHON=${python_bin}"
        --config-setting="cmake.define.FOURIER_AURORA_SDK_PYTHON_SUFFIX=${target_suffix}"
    )
fi
CMAKE_BUILD_PARALLEL_LEVEL="${JOBS:-4}" "${python_bin}" -m build "${project_root}" "${build_args[@]}"

cache="${build_dir}/CMakeCache.txt"
grep -Eq '^SKBUILD:(BOOL|STRING)=(ON|1|2)$' "${cache}"
grep -Fq "SKBUILD_PROJECT_VERSION:STRING=${version}" "${cache}"
grep -Fq "FOURIER_AURORA_SDK_DDS_PREFIX:PATH=${dds_prefix}" "${cache}"
grep -Fq 'FOURIER_AURORA_SDK_ENABLE_DDS_TRANSPORT:BOOL=ON' "${cache}"
grep -Fq 'FOURIER_AURORA_SDK_BUILD_PYTHON:BOOL=ON' "${cache}"
grep -Fq 'FOURIER_AURORA_SDK_BUILD_TESTS:BOOL=OFF' "${cache}"
grep -Fq 'FOURIER_AURORA_SDK_BUILD_CLIENT_TOOLS:BOOL=OFF' "${cache}"
grep -Fq 'FOURIER_AURORA_SDK_BUILD_EXAMPLES:BOOL=OFF' "${cache}"
grep -Fq "fourier_dds_DIR:PATH=${dds_prefix}/lib/cmake/fourier_dds" "${cache}"
grep -Fq "FOURIER_AURORA_SDK_MESSAGE_INCLUDE_DIR:PATH=${dds_prefix}/fourier_dds_msgs/include/fourier_dds_msgs" "${cache}"

if [[ "${include_commit}" == "ON" ]]; then
    expected_name="fourier_aurora_client-${version}-${build_tag}-${python_tag}-${python_tag}-linux_${wheel_arch}.whl"
else
    expected_name="fourier_aurora_client-${version}-${python_tag}-${python_tag}-linux_${wheel_arch}.whl"
fi

mapfile -t wheels < <(find "${artifact_dir}" -maxdepth 1 -type f -name "${expected_name}" -print)
if [[ "${#wheels[@]}" -ne 1 ]]; then
    echo "Expected exactly one new wheel named ${expected_name}; found ${#wheels[@]}." >&2
    exit 1
fi

echo "Built ${wheels[0]}"
echo "Build directory: ${build_dir}"
