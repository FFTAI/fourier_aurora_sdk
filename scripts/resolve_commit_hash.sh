#!/usr/bin/env bash
set -euo pipefail

project_root="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
commit="${FOURIER_AURORA_SDK_COMMIT:-}"

if [[ -z "${commit}" ]]; then
    commit="$(git -C "${project_root}" rev-parse HEAD 2>/dev/null || true)"
fi

if [[ ! "${commit}" =~ ^[0-9a-fA-F]{8,40}$ ]]; then
    echo "Unable to resolve the SDK commit; set FOURIER_AURORA_SDK_COMMIT to a Git commit hash." >&2
    exit 1
fi

printf '%s\n' "${commit:0:8}" | tr '[:upper:]' '[:lower:]'
