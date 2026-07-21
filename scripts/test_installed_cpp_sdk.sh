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
build_dir="$(mktemp -d "${build_root%/}/fourier-aurora-sdk-consumer.XXXXXX")"
trap 'rm -rf "${build_dir}"' EXIT

cmake -S "${project_root}/tests/installed" -B "${build_dir}" \
    -DFOURIER_AURORA_SDK_PREFIX="${sdk_prefix}" \
    -DCMAKE_FIND_USE_PACKAGE_REGISTRY=FALSE \
    -DCMAKE_FIND_USE_SYSTEM_PACKAGE_REGISTRY=FALSE
cmake --build "${build_dir}" --parallel "${JOBS:-4}"

library_path="${sdk_prefix}/lib:${dds_prefix}/lib:${dds_prefix}/fourier_dds_msgs/lib:/usr/local/lib"
LD_LIBRARY_PATH="${library_path}${LD_LIBRARY_PATH:+:${LD_LIBRARY_PATH}}" \
    "${build_dir}/installed_consumer"
