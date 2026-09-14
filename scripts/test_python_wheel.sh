#!/usr/bin/env bash
set -euo pipefail

project_root="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
wheel="${1:-}"
dds_prefix="${FOURIER_AURORA_SDK_DDS_PREFIX:-}"
python_bin="${PYTHON_BIN:-python3}"

if [[ -z "${wheel}" || ! -f "${wheel}" ]]; then
    echo "Usage: $0 <fourier_aurora_client.whl>" >&2
    exit 2
fi
if [[ -z "${dds_prefix}" || ! -d "${dds_prefix}" ]]; then
    echo "Set FOURIER_AURORA_SDK_DDS_PREFIX to the exact staged dependency prefix." >&2
    exit 2
fi
filename="$(basename "${wheel}")"
if [[ ! "${filename}" =~ -(cp(310|311|312|313))-(cp(310|311|312|313))-linux_(x86_64|aarch64)\.whl$ ]]; then
    echo "Cannot determine the supported CPython version from ${filename}." >&2
    exit 2
fi
wheel_python_tag="${BASH_REMATCH[1]}"
wheel_python_digits="${BASH_REMATCH[2]}"
wheel_abi_tag="${BASH_REMATCH[3]}"
wheel_arch="${BASH_REMATCH[5]}"
if [[ "$(uname -m)" != "${wheel_arch}" ]]; then
    echo "Run the wheel runtime check on a native ${wheel_arch} host." >&2
    exit 2
fi
if [[ "${wheel_python_tag}" != "${wheel_abi_tag}" ]]; then
    echo "Python and ABI tags must match: ${wheel_python_tag} != ${wheel_abi_tag}" >&2
    exit 2
fi
if ! command -v "${python_bin}" >/dev/null 2>&1; then
    echo "Python interpreter is unavailable: ${python_bin}" >&2
    exit 2
fi
interpreter_tag="$("${python_bin}" -c 'import platform, sys; assert platform.python_implementation() == "CPython"; print(f"cp{sys.version_info.major}{sys.version_info.minor}")')"
if [[ "${interpreter_tag}" != "${wheel_python_tag}" ]]; then
    echo "${filename} requires ${wheel_python_tag}, but ${python_bin} is ${interpreter_tag}." >&2
    exit 2
fi
soversion="$(sed -nE 's/^[[:space:]]*set\(FOURIER_AURORA_SDK_SOVERSION[[:space:]]+([0-9]+)\).*$/\1/p' "${project_root}/CMakeLists.txt")"
if [[ -z "${soversion}" ]]; then
    echo "Unable to read FOURIER_AURORA_SDK_SOVERSION." >&2
    exit 1
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

"${python_bin}" -m venv "${test_root}/venv"
"${test_root}/venv/bin/python" -m pip install \
    --disable-pip-version-check --no-index --no-deps "${wheel}"

site_packages="$("${test_root}/venv/bin/python" -c 'import sysconfig; print(sysconfig.get_path("platlib"))')"
package_dir="${site_packages}/fourier_aurora_client"
extension="${package_dir}/_fourier_aurora_sdk.cpython-${wheel_python_digits}-${wheel_arch}-linux-gnu.so"
sdk_library="${package_dir}/libfourier_aurora_sdk.so.${soversion}"
loader_path="${dds_prefix}/lib:${dds_prefix}/fourier_dds_msgs/lib:/usr/local/lib"

extension_loader="$(LD_LIBRARY_PATH="${loader_path}" ldd "${extension}")"
sdk_loader="$(LD_LIBRARY_PATH="${loader_path}" ldd "${sdk_library}")"
if grep -Fq 'not found' <<< "${extension_loader}${sdk_loader}"; then
    echo "Wheel has unresolved native dependencies." >&2
    echo "${extension_loader}${sdk_loader}" >&2
    exit 1
fi
sdk_resolution="$(grep -F "libfourier_aurora_sdk.so.${soversion} =>" <<< "${extension_loader}" || true)"
core_resolution="$(grep -F 'libfourierdds_core.so =>' <<< "${sdk_loader}" || true)"
state_resolution="$(grep -F 'libAuroraState.so =>' <<< "${sdk_loader}" || true)"
if [[ "${sdk_resolution}" != *"=> ${package_dir}/libfourier_aurora_sdk.so.${soversion}"* ||
      "${core_resolution}" != *"=> ${dds_prefix}/"* ||
      "${state_resolution}" != *"=> ${dds_prefix}/fourier_dds_msgs/lib/"* ]]; then
    echo "Wheel native libraries did not resolve from the isolated wheel and staged prefix." >&2
    echo "${extension_loader}${sdk_loader}" >&2
    exit 1
fi
if grep -Eq '=> /opt/fftai/' <<< "${extension_loader}${sdk_loader}"; then
    echo "Wheel test resolved a native dependency from the persistent /opt installation." >&2
    echo "${extension_loader}${sdk_loader}" >&2
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
