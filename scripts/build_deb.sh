#!/usr/bin/env bash
set -euo pipefail

project_root="$(cd "$(dirname "$0")/.." && pwd)"
include_commit="${INCLUDE_COMMIT_HASH:-OFF}"
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
        ;;
    OFF)
        commit=""
        ;;
    *)
        echo "INCLUDE_COMMIT_HASH must be ON or OFF." >&2
        exit 2
        ;;
esac

build_root="${BUILD_ROOT:-/tmp}"
artifact_root="${ARTIFACT_ROOT:-/tmp}"
require_external_root BUILD_ROOT "${build_root}"
require_external_root ARTIFACT_ROOT "${artifact_root}"
mkdir -p "${build_root}" "${artifact_root}"
build_dir="$(mktemp -d "${build_root%/}/fourier-aurora-sdk-deb-build.XXXXXX")"
artifact_dir="$(mktemp -d "${artifact_root%/}/fourier-aurora-sdk-deb-artifacts.XXXXXX")"

cmake -S "${project_root}" -B "${build_dir}" \
    -DCMAKE_BUILD_TYPE=Release \
    -DCMAKE_INSTALL_PREFIX=/opt/fftai/fourier_aurora_sdk \
    -DFOURIER_AURORA_SDK_DDS_PREFIX="${dds_prefix}" \
    -DFOURIER_AURORA_SDK_BUILD_PYTHON=OFF \
    -DFOURIER_AURORA_SDK_BUILD_TESTS=OFF \
    -DFOURIER_AURORA_SDK_BUILD_CLIENT_TOOLS=ON \
    -DFOURIER_AURORA_SDK_INSTALL_SYSTEM_CLIENT=ON \
    -DFOURIER_AURORA_SDK_BUILD_EXAMPLES=ON \
    -DFOURIER_AURORA_SDK_ENABLE_DDS_TRANSPORT=ON \
    -DFOURIER_AURORA_SDK_PACKAGE_COMMIT="${commit}"

cache="${build_dir}/CMakeCache.txt"
grep -Fq "FOURIER_AURORA_SDK_DDS_PREFIX:PATH=${dds_prefix}" "${cache}"
grep -Fq 'FOURIER_AURORA_SDK_ENABLE_DDS_TRANSPORT:BOOL=ON' "${cache}"
grep -Fq 'FOURIER_AURORA_SDK_BUILD_PYTHON:BOOL=OFF' "${cache}"
grep -Fq 'FOURIER_AURORA_SDK_BUILD_TESTS:BOOL=OFF' "${cache}"
grep -Fq 'FOURIER_AURORA_SDK_BUILD_CLIENT_TOOLS:BOOL=ON' "${cache}"
grep -Fq 'FOURIER_AURORA_SDK_INSTALL_SYSTEM_CLIENT:BOOL=ON' "${cache}"
grep -Fq 'FOURIER_AURORA_SDK_BUILD_EXAMPLES:BOOL=ON' "${cache}"
grep -Fq "fourier_dds_DIR:PATH=${dds_prefix}/lib/cmake/fourier_dds" "${cache}"
grep -Fq "FOURIER_AURORA_SDK_MESSAGE_INCLUDE_DIR:PATH=${dds_prefix}/fourier_dds_msgs/include/fourier_dds_msgs" "${cache}"

cmake --build "${build_dir}" --parallel "${JOBS:-4}"
cmake --build "${build_dir}" --target package

version="$(tr -d '[:space:]' < "${project_root}/VERSION")"
if [[ "${include_commit}" == "ON" ]]; then
    expected_name="fourier-aurora-sdk_${version}+g${commit}_*.deb"
else
    expected_name="fourier-aurora-sdk_${version}_*.deb"
fi

mapfile -t packages < <(find "${build_dir}" -maxdepth 1 -type f -name "${expected_name}" -print)
if [[ "${#packages[@]}" -ne 1 ]]; then
    echo "Expected exactly one new Debian artifact matching ${expected_name}; found ${#packages[@]}." >&2
    exit 1
fi
package="${artifact_dir}/$(basename "${packages[0]}")"
cp -- "${packages[0]}" "${package}"
case "${ARCH:-$(uname -m)}" in
    amd64|x86_64) expected_arch=amd64; expected_machine="Advanced Micro Devices X86-64" ;;
    arm64|aarch64) expected_arch=arm64; expected_machine=AArch64 ;;
    *) echo "Unsupported package architecture: ${ARCH:-}" >&2; exit 2 ;;
esac
[[ "$(dpkg-deb -f "${package}" Architecture)" == "${expected_arch}" ]]
check_dir="$(mktemp -d "${build_root%/}/sdk-deb-check.XXXXXX")"
trap 'rm -rf "${check_dir}"' EXIT
dpkg-deb -x "${package}" "${check_dir}"
elf_count=0
while IFS= read -r -d '' binary; do
    if header="$(readelf -h "${binary}" 2>/dev/null)"; then
        grep -Fq "${expected_machine}" <<< "${header}"
        elf_count=$((elf_count + 1))
    fi
done < <(find "${check_dir}" -type f -print0)
[[ "${elf_count}" -gt 0 ]]
echo "DEB architecture verified: ${expected_arch}, ${elf_count} ELF files"

echo "Built ${package}"
echo "Build directory: ${build_dir}"
