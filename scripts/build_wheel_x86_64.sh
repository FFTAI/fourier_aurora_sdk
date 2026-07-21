#!/usr/bin/env bash
set -euo pipefail

project_root="$(cd "$(dirname "$0")/.." && pwd)"
include_commit="${INCLUDE_COMMIT_HASH:-ON}"
dds_prefix="${FOURIER_AURORA_SDK_DDS_PREFIX:-}"

require_external_root() {
    local label="$1"
    local root
    root="$(realpath -m "$2")"
    if [[ "${root}" == "${project_root}" || "${root}" == "${project_root}/"* ]]; then
        echo "${label} must be outside the SDK source tree: ${root}" >&2
        exit 2
    fi
}

if [[ "$(uname -m)" != "x86_64" ]]; then
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

python_version="$(python3 -c 'import sys; print(f"{sys.version_info.major}{sys.version_info.minor}")')"
if [[ "${python_version}" != "310" ]]; then
    echo "Stage-one wheel builds require CPython 3.10; found cp${python_version}." >&2
    exit 2
fi
if ! python3 -c 'import build, pybind11, scikit_build_core' >/dev/null 2>&1; then
    echo "Install build, pybind11, and scikit-build-core in the active Python environment." >&2
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

python3 -m build "${project_root}" "${build_args[@]}"

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
    expected_name="fourier_aurora_client-${version}-${build_tag}-cp310-cp310-linux_x86_64.whl"
else
    expected_name="fourier_aurora_client-${version}-cp310-cp310-linux_x86_64.whl"
fi

mapfile -t wheels < <(find "${artifact_dir}" -maxdepth 1 -type f -name "${expected_name}" -print)
if [[ "${#wheels[@]}" -ne 1 ]]; then
    echo "Expected exactly one new wheel named ${expected_name}; found ${#wheels[@]}." >&2
    exit 1
fi

echo "Built ${wheels[0]}"
echo "Build directory: ${build_dir}"
