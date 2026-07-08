#!/usr/bin/env bash
set -euo pipefail

cd "$(dirname "$0")/.."

BUILD_DIR="${BUILD_DIR:-build_deb}"
cmake -S . -B "${BUILD_DIR}" \
  -DCMAKE_PREFIX_PATH="${CMAKE_PREFIX_PATH:-/opt/fftai}" \
  -DCMAKE_INSTALL_PREFIX=/opt/fftai/fourier_aurora_sdk \
  -DFOURIER_AURORA_SDK_BUILD_PYTHON=OFF \
  -DFOURIER_AURORA_SDK_ENABLE_DDS_TRANSPORT=ON
cmake --build "${BUILD_DIR}" -j"${JOBS:-4}"
cmake --build "${BUILD_DIR}" --target package
