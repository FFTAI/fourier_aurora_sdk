#!/usr/bin/env bash
set -euo pipefail

repo_root="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
source "${FCB_HELPER_DIR:?FCB_HELPER_DIR is required}/fcb_entrypoint.sh"
fcb_init fourier_aurora_sdk "${repo_root}"

if fcb_dry_run "scripts/build_packages.sh arch=${FCB_ARCH} jobs=${FCB_JOBS}"; then
    exit 0
fi

# Adapt the shared FCB contract to the module's standalone packaging scripts.
if fcb_is_arm64; then
    export CMAKE_TOOLCHAIN_FILE="${FCB_TOOLCHAIN_FILE:-${CMAKE_TOOLCHAIN_FILE:?ARM packaging requires FCB_TOOLCHAIN_FILE or the image CMAKE_TOOLCHAIN_FILE}}"
    export FOURIER_AURORA_SDK_DDS_PREFIX="${FOURIER_AURORA_SDK_DDS_PREFIX:-${FCB_SYSROOT}/opt/fftai}"
else
    export FOURIER_AURORA_SDK_DDS_PREFIX="${FOURIER_AURORA_SDK_DDS_PREFIX:-/opt/fftai}"
fi

bash "${repo_root}/scripts/build_packages.sh"
fcb_collect_artifacts deb "${repo_root}/dist"
fcb_collect_artifacts whl "${repo_root}/dist"
