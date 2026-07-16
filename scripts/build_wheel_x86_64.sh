#!/usr/bin/env bash
set -euo pipefail

project_root="$(cd "$(dirname "$0")/.." && pwd)"
include_commit="${INCLUDE_COMMIT_HASH:-ON}"
build_args=(--wheel)

case "${include_commit}" in
    ON)
        commit="$(${project_root}/scripts/resolve_commit_hash.sh)"
        build_tag="1g${commit}"
        build_args+=(--config-setting="wheel.build-tag=${build_tag}")
        ;;
    OFF)
        commit=""
        build_tag=""
        ;;
    *)
        echo "INCLUDE_COMMIT_HASH must be ON or OFF." >&2
        exit 2
        ;;
esac

cd "${project_root}"
python3 -m build "${build_args[@]}"

if [[ "${include_commit}" == "ON" ]]; then
    wheel="$(find dist -maxdepth 1 -type f -name "*-${build_tag}-*.whl" -print -quit)"
else
    version="$(tr -d '[:space:]' < VERSION)"
    wheel="$(find dist -maxdepth 1 -type f -name "fourier_aurora_client-${version}-cp*.whl" -print -quit)"
fi
if [[ -z "${wheel}" ]]; then
    echo "Wheel build completed without the expected artifact name." >&2
    exit 1
fi
if [[ "${include_commit}" == "ON" ]]; then
    echo "Built ${wheel} (SDK commit ${commit})"
else
    echo "Built public release artifact ${wheel}"
fi
