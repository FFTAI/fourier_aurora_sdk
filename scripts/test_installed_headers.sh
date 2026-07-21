#!/usr/bin/env bash
set -euo pipefail

sdk_prefix="${SDK_PREFIX:-}"
if [[ -z "${sdk_prefix}" || ! -d "${sdk_prefix}/include/fourier_aurora_sdk" ]]; then
    echo "Set SDK_PREFIX to an extracted SDK installation." >&2
    exit 2
fi

build_root="${BUILD_ROOT:-/tmp}"
mkdir -p "${build_root}"
build_dir="$(mktemp -d "${build_root%/}/fourier-aurora-sdk-headers.XXXXXX")"
trap 'rm -rf "${build_dir}"' EXIT

mapfile -t headers < <(
    find "${sdk_prefix}/include/fourier_aurora_sdk" -type f -name '*.hpp' -print | sort
)
if [[ "${#headers[@]}" -eq 0 ]]; then
    echo "No installed SDK headers were found." >&2
    exit 1
fi

for header in "${headers[@]}"; do
    relative="${header#${sdk_prefix}/include/}"
    source_file="${build_dir}/header.cpp"
    printf '#include <%s>\nint main() { return 0; }\n' "${relative}" > "${source_file}"
    g++ -std=c++17 -fsyntax-only -I"${sdk_prefix}/include" "${source_file}"
done

echo "Standalone-compiled ${#headers[@]} installed public headers."
