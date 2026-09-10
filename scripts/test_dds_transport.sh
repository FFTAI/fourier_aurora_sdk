#!/usr/bin/env bash
set -euo pipefail

project_root="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
dds_prefix="${FOURIER_AURORA_SDK_DDS_PREFIX:-}"
jobs="${JOBS:-$(nproc)}"
test_tmp_root="${TEST_TMP_ROOT:-/tmp}"

if [[ -z "${dds_prefix}" ]]; then
    echo "Set FOURIER_AURORA_SDK_DDS_PREFIX to the pinned staged dependency prefix." >&2
    exit 2
fi
if [[ ! -f "${dds_prefix}/lib/cmake/fourier_dds/fourier_ddsConfig.cmake" ]]; then
    echo "Pinned FourierDDS CMake package is missing under ${dds_prefix}." >&2
    exit 1
fi
if [[ ! -f "${dds_prefix}/fourier_dds_msgs/include/fourier_dds_msgs/AuroraState/AuroraState.hpp" ]]; then
    echo "Pinned generated message headers are missing under ${dds_prefix}." >&2
    exit 1
fi
for command in cmake ctest "${CXX:-c++}"; do
    if ! command -v "${command}" >/dev/null 2>&1; then
        echo "Required DDS test command is unavailable: ${command}" >&2
        exit 1
    fi
done

mkdir -p "${test_tmp_root}"
build_dir="$(mktemp -d "${test_tmp_root%/}/fourier_aurora_sdk_dds.XXXXXX")"

echo "Aurora SDK test profile: dds-transport"
echo "  build_dir=${build_dir}"
echo "  dds_prefix=${dds_prefix}"
echo "  tests=ON"
echo "  python=OFF"
echo "  dds=ON"
echo "  client_tools=OFF"
echo "  examples=OFF"

cmake -S "${project_root}" -B "${build_dir}" \
    -DCMAKE_BUILD_TYPE=Debug \
    -DCMAKE_EXPORT_COMPILE_COMMANDS=ON \
    -DFOURIER_AURORA_SDK_DDS_PREFIX="${dds_prefix}" \
    -DFOURIER_AURORA_SDK_BUILD_TESTS=ON \
    -DFOURIER_AURORA_SDK_BUILD_PYTHON=OFF \
    -DFOURIER_AURORA_SDK_BUILD_CLIENT_TOOLS=OFF \
    -DFOURIER_AURORA_SDK_BUILD_EXAMPLES=OFF \
    -DFOURIER_AURORA_SDK_ENABLE_DDS_TRANSPORT=ON

grep -Fq "fourier_dds_DIR:PATH=${dds_prefix}/lib/cmake/fourier_dds" \
    "${build_dir}/CMakeCache.txt" || {
        echo "CMake did not resolve FourierDDS from the staged prefix." >&2
        exit 1
    }
grep -Fq "FOURIER_AURORA_SDK_MESSAGE_INCLUDE_DIR:PATH=${dds_prefix}/fourier_dds_msgs/include/fourier_dds_msgs" \
    "${build_dir}/CMakeCache.txt" || {
        echo "CMake did not resolve generated messages from the staged prefix." >&2
        exit 1
    }

cmake --build "${build_dir}" --parallel "${jobs}"

# Explicit test options must override these conflicting environment defaults.
FOURIERDDS_ROS_COMPATIBLE=1 \
FOURIERDDS_USE_DISCOVERY_SERVER=1 \
    ctest --test-dir "${build_dir}" --output-on-failure

echo "Passed dds-transport; build retained at ${build_dir}"
