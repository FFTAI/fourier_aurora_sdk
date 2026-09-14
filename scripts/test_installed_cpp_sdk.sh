#!/usr/bin/env bash
set -euo pipefail

project_root="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
sdk_prefix="${SDK_PREFIX:-}"
dds_prefix="${FOURIER_AURORA_SDK_DDS_PREFIX:-}"

if [[ -z "${sdk_prefix}" || -z "${dds_prefix}" ]]; then
    echo "Set SDK_PREFIX and FOURIER_AURORA_SDK_DDS_PREFIX to isolated staged prefixes." >&2
    exit 2
fi
if [[ ! -d "${sdk_prefix}" || ! -d "${dds_prefix}" ]]; then
    echo "The staged SDK or DDS dependency prefix does not exist." >&2
    exit 2
fi

build_root="${BUILD_ROOT:-/tmp}"
mkdir -p "${build_root}"
test_root="$(mktemp -d "${build_root%/}/fourier-aurora-sdk-consumer.XXXXXX")"
peer_pid=""
cleanup() {
    if [[ -n "${peer_pid}" ]] && kill -0 "${peer_pid}" 2>/dev/null; then
        kill "${peer_pid}" 2>/dev/null || true
        wait "${peer_pid}" 2>/dev/null || true
    fi
    rm -rf "${test_root}"
}
trap cleanup EXIT

cmake -S "${project_root}/tests/installed" -B "${test_root}/consumer-build" \
    -DFOURIER_AURORA_SDK_PREFIX="${sdk_prefix}" \
    -DCMAKE_EXE_LINKER_FLAGS="-Wl,-rpath-link,${dds_prefix}/lib -Wl,-rpath-link,${dds_prefix}/fourier_dds_msgs/lib" \
    -DCMAKE_FIND_USE_PACKAGE_REGISTRY=FALSE \
    -DCMAKE_FIND_USE_SYSTEM_PACKAGE_REGISTRY=FALSE
cmake --build "${test_root}/consumer-build" --parallel "${JOBS:-4}"

cmake -S "${project_root}" -B "${test_root}/peer-build" \
    -DCMAKE_BUILD_TYPE=Release \
    -DFOURIER_AURORA_SDK_DDS_PREFIX="${dds_prefix}" \
    -DFOURIER_AURORA_SDK_ENABLE_DDS_TRANSPORT=ON \
    -DFOURIER_AURORA_SDK_BUILD_PYTHON=OFF \
    -DFOURIER_AURORA_SDK_BUILD_TESTS=ON \
    -DFOURIER_AURORA_SDK_BUILD_CLIENT_TOOLS=OFF \
    -DFOURIER_AURORA_SDK_BUILD_EXAMPLES=OFF
cmake --build "${test_root}/peer-build" \
    --target dds_wheel_peer --parallel "${JOBS:-4}"

library_path="${sdk_prefix}/lib:${dds_prefix}/lib:${dds_prefix}/fourier_dds_msgs/lib:/usr/local/lib"
LD_LIBRARY_PATH="${library_path}${LD_LIBRARY_PATH:+:${LD_LIBRARY_PATH}}" \
    "${test_root}/consumer-build/installed_consumer"

LD_LIBRARY_PATH="${library_path}${LD_LIBRARY_PATH:+:${LD_LIBRARY_PATH}}" \
FOURIER_DDS_DOMAIN_ID=87 \
FOURIER_DDS_NAMESPACE=hostile_namespace \
    "${test_root}/peer-build/dds_wheel_peer" \
    >"${test_root}/peer.log" 2>&1 &
peer_pid=$!

LD_LIBRARY_PATH="${library_path}${LD_LIBRARY_PATH:+:${LD_LIBRARY_PATH}}" \
FOURIER_DDS_DOMAIN_ID=87 \
FOURIER_DDS_NAMESPACE=hostile_namespace \
    "${test_root}/consumer-build/installed_consumer" --dds-loopback

if ! wait "${peer_pid}"; then
    cat "${test_root}/peer.log" >&2
    exit 1
fi
peer_pid=""
cat "${test_root}/peer.log"
echo "Installed C++ SDK loader and DDS loopback checks passed."
