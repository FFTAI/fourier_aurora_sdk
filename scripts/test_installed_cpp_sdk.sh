#!/usr/bin/env bash
set -euo pipefail

SDK_PREFIX="${SDK_PREFIX:-/opt/fftai/fourier_aurora_sdk}"
SRC="${1:-${SDK_PREFIX}/share/doc/fourier_aurora_sdk/examples/cpp/installed_smoke.cpp}"
OUT="${OUT:-/tmp/fourier_aurora_sdk_installed_smoke}"

g++ -std=c++17 "${SRC}" \
  -I"${SDK_PREFIX}/include" \
  -L"${SDK_PREFIX}/lib" \
  -Wl,-rpath,"${SDK_PREFIX}/lib" \
  -Wl,-rpath,/opt/fftai/fourier_dds_msgs/lib \
  -Wl,-rpath,/usr/local/lib \
  -lfourier_aurora_sdk \
  -o "${OUT}"

"${OUT}"
