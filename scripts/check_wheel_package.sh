#!/usr/bin/env bash
set -euo pipefail

project_root="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
wheel="${1:-}"
expected_platform="${EXPECTED_PLATFORM:-linux_x86_64}"
case "${expected_platform}" in
    linux_x86_64) gnu_arch="x86_64" ;;
    linux_aarch64) gnu_arch="aarch64" ;;
    *)
        echo "EXPECTED_PLATFORM must be linux_x86_64 or linux_aarch64." >&2
        exit 2
        ;;
esac
if [[ -z "${wheel}" || ! -f "${wheel}" ]]; then
    echo "Usage: $0 <fourier_aurora_client.whl>" >&2
    exit 2
fi

version="$(tr -d '[:space:]' < "${project_root}/VERSION")"
version_regex="${version//./\\.}"
filename="$(basename "${wheel}")"
if [[ ! "${filename}" =~ ^fourier_aurora_client-${version_regex}(-1g[0-9a-f]{8})?-(cp(310|311|312|313))-(cp(310|311|312|313))-linux_(x86_64|aarch64)\.whl$ ]]; then
    echo "Unexpected wheel filename: ${filename}" >&2
    exit 1
fi
python_tag="${BASH_REMATCH[2]}"
python_digits="${BASH_REMATCH[3]}"
abi_tag="${BASH_REMATCH[4]}"
platform_tag="linux_${BASH_REMATCH[7]}"
if [[ "${python_tag}" != "${abi_tag}" ]]; then
    echo "Python and ABI tags must match: ${python_tag} != ${abi_tag}" >&2
    exit 1
fi
if [[ "${platform_tag}" != "${expected_platform}" ]]; then
    echo "Wheel platform ${platform_tag} does not match expected ${expected_platform}." >&2
    exit 1
fi
soversion="$(sed -nE 's/^[[:space:]]*set\(FOURIER_AURORA_SDK_SOVERSION[[:space:]]+([0-9]+)\).*$/\1/p' "${project_root}/CMakeLists.txt")"
if [[ -z "${soversion}" ]]; then
    echo "Unable to read FOURIER_AURORA_SDK_SOVERSION." >&2
    exit 1
fi

build_root="${BUILD_ROOT:-/tmp}"
mkdir -p "${build_root}"
extract_root="$(mktemp -d "${build_root%/}/fourier-aurora-sdk-wheel-check.XXXXXX")"
trap 'rm -rf "${extract_root}"' EXIT
unzip -q "${wheel}" -d "${extract_root}"

package_dir="${extract_root}/fourier_aurora_client"
dist_info="${extract_root}/fourier_aurora_client-${version}.dist-info"
required_files=(
    "${package_dir}/__init__.py"
    "${package_dir}/client.py"
    "${package_dir}/_fourier_aurora_sdk.cpython-${python_digits}-${gnu_arch}-linux-gnu.so"
    "${package_dir}/libfourier_aurora_sdk.so.${soversion}"
    "${dist_info}/METADATA"
    "${dist_info}/WHEEL"
    "${dist_info}/RECORD"
    "${dist_info}/licenses/LICENSE"
)
for required_file in "${required_files[@]}"; do
    if [[ ! -f "${required_file}" ]]; then
        echo "Wheel is missing ${required_file#${extract_root}/}." >&2
        exit 1
    fi
done

mapfile -t sdk_libraries < <(find "${package_dir}" -maxdepth 1 -type f -name 'libfourier_aurora_sdk.so*' -print)
if [[ "${#sdk_libraries[@]}" -ne 1 ]]; then
    echo "Wheel must contain exactly one private SDK shared library." >&2
    exit 1
fi
cmp "${project_root}/LICENSE" "${dist_info}/licenses/LICENSE"
grep -Fq 'Name: fourier-aurora-client' "${dist_info}/METADATA"
grep -Fq "Version: ${version}" "${dist_info}/METADATA"
grep -Fq 'Requires-Python: <3.14,>=3.10' "${dist_info}/METADATA"
grep -Fq "Tag: ${python_tag}-${abi_tag}-${expected_platform}" "${dist_info}/WHEEL"

if find "${extract_root}" -type f -print | grep -Eq '(/examples?/|/tests?/|mock_transport|client_tools|/include/)'; then
    echo "Wheel contains examples, tests, tools, or headers." >&2
    exit 1
fi
while IFS= read -r entry; do
    relative="${entry#${extract_root}/}"
    if [[ "${relative}" != fourier_aurora_client/* &&
          "${relative}" != fourier_aurora_client-${version}.dist-info/* ]]; then
        echo "Unexpected top-level wheel entry: ${relative}" >&2
        exit 1
    fi
done < <(find "${extract_root}" -type f -print)

extension="${package_dir}/_fourier_aurora_sdk.cpython-${python_digits}-${gnu_arch}-linux-gnu.so"
sdk_library="${package_dir}/libfourier_aurora_sdk.so.${soversion}"
case "${expected_platform}:$(readelf -h "${extension}" | sed -n 's/^[[:space:]]*Machine:[[:space:]]*//p')" in
    linux_x86_64:*X86-64*|linux_aarch64:*AArch64*) ;;
    *)
        echo "Extension ELF machine does not match ${expected_platform}." >&2
        exit 1
        ;;
esac
extension_dynamic="$(readelf -d "${extension}")"
sdk_dynamic="$(readelf -d "${sdk_library}")"
grep -Fq '(RUNPATH)' <<< "${extension_dynamic}"
grep -Fq '$ORIGIN' <<< "${extension_dynamic}"
grep -Fq "Shared library: [libfourier_aurora_sdk.so.${soversion}]" <<< "${extension_dynamic}"
grep -Fq '(RUNPATH)' <<< "${sdk_dynamic}"
grep -Fq '$ORIGIN' <<< "${sdk_dynamic}"
grep -Fq '/opt/fftai/fourier_dds_msgs/lib' <<< "${sdk_dynamic}"
grep -Fq 'Shared library: [libfourierdds_core.so]' <<< "${sdk_dynamic}"
grep -Fq 'Shared library: [libAuroraState.so]' <<< "${sdk_dynamic}"
if grep -Fq '(RPATH)' <<< "${extension_dynamic}${sdk_dynamic}"; then
    echo "Wheel ELF objects contain DT_RPATH instead of DT_RUNPATH." >&2
    exit 1
fi
if grep -Eq '/tmp/|/workspace/' <<< "${extension_dynamic}${sdk_dynamic}"; then
    echo "Wheel ELF objects contain a build-tree path." >&2
    exit 1
fi
if nm -D "${extension}" "${sdk_library}" | grep -Eq 'installTest|MockTransport|FOURIER_AURORA_SDK_TESTING'; then
    echo "Wheel exports private test hooks." >&2
    exit 1
fi

echo "Wheel passed filename, metadata, content, license, and ELF checks."
