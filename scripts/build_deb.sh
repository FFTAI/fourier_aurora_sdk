#!/usr/bin/env bash
set -euo pipefail

project_root="$(cd "$(dirname "$0")/.." && pwd)"
include_commit="${INCLUDE_COMMIT_HASH:-ON}"
dds_prefix="${FOURIER_AURORA_SDK_DDS_PREFIX:-}"
target_arch="${ARCH:-amd64}"
case "${target_arch}" in
    amd64|x86_64) target_arch=amd64 ;;
    arm64|aarch64) target_arch=arm64 ;;
    *)
        echo "Unsupported target architecture: ${target_arch}" >&2
        exit 2
        ;;
esac
# 交叉构建时 find_* 的显式 PATHS 会在 FIND_ROOT_PATH 模式下 re-root 到
# sysroot，因此解析结果以 sysroot 前缀为准（amd64 原生构建为空串）。
resolved_prefix=""
if [[ "${target_arch}" == "arm64" ]]; then
    effective_sysroot="${FCB_SYSROOT:-${USER_SYSROOT:-${SYSROOT:-}}}"
    if [[ -z "${effective_sysroot}" || ! -d "${effective_sysroot}" ]]; then
        echo "arm64 cross build requires FCB_SYSROOT/USER_SYSROOT/SYSROOT pointing at the arm64 sysroot." >&2
        exit 2
    fi
    resolved_prefix="${effective_sysroot%/}"
fi

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

cmake_configure_args=(
    -DCMAKE_BUILD_TYPE=Release
    -DCMAKE_INSTALL_PREFIX=/opt/fftai/fourier_aurora_sdk
    -DFOURIER_AURORA_SDK_DDS_PREFIX="${dds_prefix}"
    -DFOURIER_AURORA_SDK_BUILD_PYTHON=OFF
    -DFOURIER_AURORA_SDK_BUILD_TESTS=OFF
    -DFOURIER_AURORA_SDK_BUILD_CLIENT_TOOLS=ON
    -DFOURIER_AURORA_SDK_INSTALL_SYSTEM_CLIENT=ON
    -DFOURIER_AURORA_SDK_BUILD_EXAMPLES=ON
    -DFOURIER_AURORA_SDK_ENABLE_DDS_TRANSPORT=ON
    -DFOURIER_AURORA_SDK_PACKAGE_COMMIT="${commit}"
)
if [[ "${target_arch}" == "arm64" ]]; then
    toolchain_file="${CMAKE_TOOLCHAIN_FILE:-${project_root}/toolchains/aarch64-linux-gnu.cmake}"
    if [[ ! -f "${toolchain_file}" ]]; then
        echo "arm64 cross toolchain file is missing: ${toolchain_file}" >&2
        exit 2
    fi
    cmake_configure_args+=(-DCMAKE_TOOLCHAIN_FILE="${toolchain_file}")
fi
cmake -S "${project_root}" -B "${build_dir}" "${cmake_configure_args[@]}"

cache="${build_dir}/CMakeCache.txt"
grep -Fq "FOURIER_AURORA_SDK_DDS_PREFIX:PATH=${dds_prefix}" "${cache}"
grep -Fq 'FOURIER_AURORA_SDK_ENABLE_DDS_TRANSPORT:BOOL=ON' "${cache}"
grep -Fq 'FOURIER_AURORA_SDK_BUILD_PYTHON:BOOL=OFF' "${cache}"
grep -Fq 'FOURIER_AURORA_SDK_BUILD_TESTS:BOOL=OFF' "${cache}"
grep -Fq 'FOURIER_AURORA_SDK_BUILD_CLIENT_TOOLS:BOOL=ON' "${cache}"
grep -Fq 'FOURIER_AURORA_SDK_INSTALL_SYSTEM_CLIENT:BOOL=ON' "${cache}"
grep -Fq 'FOURIER_AURORA_SDK_BUILD_EXAMPLES:BOOL=ON' "${cache}"
grep -Fq "fourier_dds_DIR:PATH=${resolved_prefix}${dds_prefix}/lib/cmake/fourier_dds" "${cache}"
grep -Fq "FOURIER_AURORA_SDK_MESSAGE_INCLUDE_DIR:PATH=${resolved_prefix}${dds_prefix}/fourier_dds_msgs/include/fourier_dds_msgs" "${cache}"

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
# Architecture 显式核对（amc !60 同款诊断：#241 交叉构建曾打出与目标不符的 arch）。
deb_arch="$(dpkg-deb -f "${packages[0]}" Architecture)"
cpack_conf_arch="$(sed -n 's/^set(CPACK_DEBIAN_PACKAGE_ARCHITECTURE "\(.*\)")$/\1/p' "${build_dir}/CPackConfig.cmake")"
printf 'arch check: deb=%s expected=%s cpack_config=%s FCB_ARCH=%s ARCH=%s\n' \
    "${deb_arch}" "${target_arch}" "${cpack_conf_arch}" "${FCB_ARCH:-}" "${ARCH:-}" >&2
if [[ "${deb_arch}" != "${target_arch}" ]]; then
    echo "DEB Architecture mismatch: deb=${deb_arch} expected=${target_arch}" >&2
    exit 1
fi
package="${artifact_dir}/$(basename "${packages[0]}")"
cp -- "${packages[0]}" "${package}"

echo "Built ${package}"
echo "Build directory: ${build_dir}"
