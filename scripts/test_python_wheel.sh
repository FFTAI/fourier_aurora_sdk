#!/usr/bin/env bash
set -euo pipefail

project_root="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
wheel="${1:-}"
dds_prefix="${FOURIER_AURORA_SDK_DDS_PREFIX:-}"

if [[ -z "${wheel}" || ! -f "${wheel}" ]]; then
    echo "Usage: $0 <fourier_aurora_client.whl>" >&2
    exit 2
fi
if [[ -z "${dds_prefix}" || ! -d "${dds_prefix}" ]]; then
    echo "Set FOURIER_AURORA_SDK_DDS_PREFIX to the exact staged dependency prefix." >&2
    exit 2
fi

build_root="${BUILD_ROOT:-/tmp}"
mkdir -p "${build_root}"
test_root="$(mktemp -d "${build_root%/}/fourier-aurora-sdk-wheel-test.XXXXXX")"
peer_pid=""
cleanup() {
    if [[ -n "${peer_pid}" ]] && kill -0 "${peer_pid}" 2>/dev/null; then
        kill "${peer_pid}" 2>/dev/null || true
        wait "${peer_pid}" 2>/dev/null || true
    fi
    rm -rf "${test_root}"
}
trap cleanup EXIT

cmake -S "${project_root}" -B "${test_root}/build" \
    -DCMAKE_BUILD_TYPE=Release \
    -DFOURIER_AURORA_SDK_DDS_PREFIX="${dds_prefix}" \
    -DFOURIER_AURORA_SDK_ENABLE_DDS_TRANSPORT=ON \
    -DFOURIER_AURORA_SDK_BUILD_PYTHON=OFF \
    -DFOURIER_AURORA_SDK_BUILD_TESTS=ON \
    -DFOURIER_AURORA_SDK_BUILD_CLIENT_TOOLS=OFF \
    -DFOURIER_AURORA_SDK_BUILD_EXAMPLES=OFF
cmake --build "${test_root}/build" --target dds_wheel_peer --parallel "${JOBS:-4}"

python3 -m venv "${test_root}/venv"
"${test_root}/venv/bin/python" -m pip install \
    --disable-pip-version-check --no-index --no-deps "${wheel}"

site_packages="${test_root}/venv/lib/python3.10/site-packages"
package_dir="${site_packages}/fourier_aurora_client"
extension="${package_dir}/_fourier_aurora_sdk.cpython-310-x86_64-linux-gnu.so"
sdk_library="${package_dir}/libfourier_aurora_sdk.so.3"
loader_path="${dds_prefix}/lib:${dds_prefix}/fourier_dds_msgs/lib:/usr/local/lib"

extension_loader="$(LD_LIBRARY_PATH="${loader_path}" ldd "${extension}")"
sdk_loader="$(LD_LIBRARY_PATH="${loader_path}" ldd "${sdk_library}")"
if grep -Fq 'not found' <<< "${extension_loader}${sdk_loader}"; then
    echo "Wheel has unresolved native dependencies." >&2
    echo "${extension_loader}${sdk_loader}" >&2
    exit 1
fi
sdk_resolution="$(grep -F 'libfourier_aurora_sdk.so.3 =>' <<< "${extension_loader}" || true)"
core_resolution="$(grep -F 'libfourierdds_core.so =>' <<< "${sdk_loader}" || true)"
state_resolution="$(grep -F 'libAuroraState.so =>' <<< "${sdk_loader}" || true)"
if [[ "${sdk_resolution}" != *"=> ${package_dir}/libfourier_aurora_sdk.so.3"* ||
      "${core_resolution}" != *"=> ${dds_prefix}/"* ||
      "${state_resolution}" != *"=> ${dds_prefix}/fourier_dds_msgs/lib/"* ]]; then
    echo "Wheel native libraries did not resolve from the isolated wheel and staged prefix." >&2
    echo "${extension_loader}${sdk_loader}" >&2
    exit 1
fi
if grep -Eq '=> /opt/fftai/' <<< "${extension_loader}${sdk_loader}"; then
    echo "Wheel test resolved a native dependency from the persistent /opt installation." >&2
    exit 1
fi

LD_LIBRARY_PATH="${loader_path}" \
FOURIER_DDS_DOMAIN_ID=87 \
FOURIER_DDS_NAMESPACE=hostile_namespace \
    "${test_root}/build/dds_wheel_peer" >"${test_root}/peer.log" 2>&1 &
peer_pid=$!

LD_LIBRARY_PATH="${loader_path}" \
FOURIER_DDS_DOMAIN_ID=87 \
FOURIER_DDS_NAMESPACE=hostile_namespace \
PYTHONPATH= \
    "${test_root}/venv/bin/python" \
    "${project_root}/tests/installed/wheel_loopback.py"

if ! wait "${peer_pid}"; then
    cat "${test_root}/peer.log" >&2
    exit 1
fi
peer_pid=""
cat "${test_root}/peer.log"
echo "Fresh-venv wheel import, loader, and DDS loopback checks passed."
