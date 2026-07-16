#!/usr/bin/env bash
set -euo pipefail

project_root="$(cd "$(dirname "$0")/.." && pwd)"
include_commit="${INCLUDE_COMMIT_HASH:-ON}"
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

cd "${project_root}"

BUILD_DIR="${BUILD_DIR:-build_deb}"
cmake -S . -B "${BUILD_DIR}" \
  -DCMAKE_PREFIX_PATH="${CMAKE_PREFIX_PATH:-/opt/fftai}" \
  -DCMAKE_INSTALL_PREFIX=/opt/fftai/fourier_aurora_sdk \
  -DFOURIER_AURORA_SDK_BUILD_PYTHON=OFF \
  -DFOURIER_AURORA_SDK_ENABLE_DDS_TRANSPORT=ON \
  -DFOURIER_AURORA_SDK_PACKAGE_COMMIT="${commit}"
cmake --build "${BUILD_DIR}" -j"${JOBS:-4}"
cmake --build "${BUILD_DIR}" --target package

if [[ "${include_commit}" == "ON" ]]; then
    package="$(find "${BUILD_DIR}" -maxdepth 1 -type f -name "fourier-aurora-sdk_*+g${commit}_*.deb" -print -quit)"
else
    version="$(tr -d '[:space:]' < VERSION)"
    package="$(find "${BUILD_DIR}" -maxdepth 1 -type f -name "fourier-aurora-sdk_${version}_*.deb" -print -quit)"
fi
if [[ -z "${package}" ]]; then
    echo "Debian build completed without the expected artifact name." >&2
    exit 1
fi
if [[ "${include_commit}" == "ON" ]]; then
    echo "Built ${package} (SDK commit ${commit})"
else
    echo "Built public release artifact ${package}"
fi
