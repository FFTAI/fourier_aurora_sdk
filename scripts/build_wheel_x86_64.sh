#!/usr/bin/env bash
set -euo pipefail
export FOURIER_AURORA_SDK_WHEEL_ARCH=x86_64
exec bash "$(dirname "$0")/build_wheel.sh"
