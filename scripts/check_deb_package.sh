#!/usr/bin/env bash
set -euo pipefail

project_root="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
package="${1:-}"
dds_prefix="${FOURIER_AURORA_SDK_DDS_PREFIX:-}"
expected_architecture="${EXPECTED_ARCHITECTURE:-amd64}"

if [[ -z "${package}" || ! -f "${package}" ]]; then
    echo "Usage: $0 <fourier-aurora-sdk.deb>" >&2
    exit 2
fi
if [[ -z "${dds_prefix}" || ! -d "${dds_prefix}" ]]; then
    echo "Set FOURIER_AURORA_SDK_DDS_PREFIX to the exact staged dependency prefix." >&2
    exit 2
fi
if [[ "${expected_architecture}" != "amd64" && "${expected_architecture}" != "arm64" ]]; then
    echo "EXPECTED_ARCHITECTURE must be amd64 or arm64." >&2
    exit 2
fi

version="$(tr -d '[:space:]' < "${project_root}/VERSION")"
version_regex="${version//./\\.}"
soversion="$(sed -nE 's/^[[:space:]]*set\(FOURIER_AURORA_SDK_SOVERSION[[:space:]]+([0-9]+)\).*$/\1/p' "${project_root}/CMakeLists.txt")"
if [[ -z "${soversion}" ]]; then
    echo "Unable to read FOURIER_AURORA_SDK_SOVERSION." >&2
    exit 1
fi
package_name="$(dpkg-deb -f "${package}" Package)"
package_version="$(dpkg-deb -f "${package}" Version)"
package_architecture="$(dpkg-deb -f "${package}" Architecture)"
package_dependencies="$(dpkg-deb -f "${package}" Depends)"

[[ "${package_name}" == "fourier-aurora-sdk" ]]
if [[ "${package_version}" != "${version}" &&
      ! "${package_version}" =~ ^${version_regex}\+g[0-9a-f]{8}$ ]]; then
    echo "Unexpected Debian version: ${package_version}" >&2
    exit 1
fi
[[ "${package_architecture}" == "${expected_architecture}" ]]
[[ "${package_dependencies}" == "fourier_dds (= 1.2.0-1), fourier-dds-msgs (= 0.2.0-1)" ]]

expected_filename="fourier-aurora-sdk_${package_version}_${package_architecture}.deb"
if [[ "$(basename "${package}")" != "${expected_filename}" ]]; then
    echo "Expected Debian filename ${expected_filename}." >&2
    exit 1
fi

build_root="${BUILD_ROOT:-/tmp}"
mkdir -p "${build_root}"
extract_root="$(mktemp -d "${build_root%/}/fourier-aurora-sdk-deb-check.XXXXXX")"
trap 'rm -rf "${extract_root}"' EXIT
dpkg-deb -x "${package}" "${extract_root}"

sdk_prefix="${extract_root}/opt/fftai/fourier_aurora_sdk"
doc_prefix="${sdk_prefix}/share/doc/fourier_aurora_sdk"
required_files=(
    "${sdk_prefix}/include/fourier_aurora_sdk/aurora_client.hpp"
    "${sdk_prefix}/include/fourier_aurora_sdk/transport/fourierdds_transport.hpp"
    "${sdk_prefix}/lib/libfourier_aurora_sdk.so.${version}"
    "${sdk_prefix}/lib/cmake/fourier_aurora_sdk/fourier_aurora_sdkConfig.cmake"
    "${sdk_prefix}/lib/cmake/fourier_aurora_sdk/fourier_aurora_sdkConfigVersion.cmake"
    "${sdk_prefix}/bin/aurora-client"
    "${extract_root}/usr/share/bash-completion/completions/aurora-client"
    "${extract_root}/usr/share/zsh/site-functions/_aurora-client"
    "${extract_root}/usr/share/fish/vendor_completions.d/aurora-client.fish"
    "${doc_prefix}/LICENSE"
    "${doc_prefix}/README.md"
    "${doc_prefix}/README_CN.md"
    "${doc_prefix}/CHANGELOG"
    "${doc_prefix}/REVISIONS.yaml"
    "${doc_prefix}/docs/overview.md"
    "${doc_prefix}/docs/overview_CN.md"
    "${doc_prefix}/docs/cpp_api.md"
    "${doc_prefix}/docs/cpp_api_CN.md"
    "${doc_prefix}/docs/python_api.md"
    "${doc_prefix}/docs/python_api_CN.md"
    "${doc_prefix}/docs/examples_and_clients.md"
    "${doc_prefix}/docs/examples_and_clients_CN.md"
    "${doc_prefix}/examples/gr3/cpp/get_info.cpp"
    "${doc_prefix}/examples/gr3/cpp/walk_forward.cpp"
    "${doc_prefix}/examples/gr3/cpp/swing_arms.cpp"
    "${doc_prefix}/examples/gr3/cpp/aurora_state_callback.cpp"
    "${doc_prefix}/examples/gr3/python/get_info.py"
    "${doc_prefix}/examples/gr3/python/walk_forward.py"
    "${doc_prefix}/examples/gr3/python/swing_arms.py"
    "${extract_root}/etc/ld.so.conf.d/fourier_aurora_sdk.conf"
)
for required_file in "${required_files[@]}"; do
    if [[ ! -f "${required_file}" ]]; then
        echo "Debian package is missing ${required_file#${extract_root}/}." >&2
        exit 1
    fi
done
[[ -L "${extract_root}/usr/bin/aurora-client" ]]
[[ "$(readlink "${extract_root}/usr/bin/aurora-client")" == \
    "/opt/fftai/fourier_aurora_sdk/bin/aurora-client" ]]
if find "${sdk_prefix}/bin" -type f -o -type l | \
    grep -Eq '/(state_client|motion_client|control_group_client)$'; then
    echo "Debian package contains replaced diagnostic clients." >&2
    exit 1
fi
[[ -L "${sdk_prefix}/lib/libfourier_aurora_sdk.so" ]]
[[ -L "${sdk_prefix}/lib/libfourier_aurora_sdk.so.${soversion}" ]]
cmp "${project_root}/LICENSE" "${doc_prefix}/LICENSE"

if find "${extract_root}" -type f -print | grep -Eq '(_fourier_aurora_sdk|mock_transport|/tests?/|__pycache__|\.pyc$|/docs/internal/|/CONTEXT\.md$|/ci/)'; then
    echo "Debian package contains Python, test, or private maintainer content." >&2
    exit 1
fi
if grep -ERq 'find_dependency\(fourier_dds|FOURIER_AURORA_SDK_TESTING|mock_transport' \
    "${sdk_prefix}/include" "${sdk_prefix}/lib/cmake"; then
    echo "Installed headers or CMake exports leak private dependencies." >&2
    exit 1
fi

sdk_library="${sdk_prefix}/lib/libfourier_aurora_sdk.so.${version}"
dynamic_section="$(readelf -d "${sdk_library}")"
grep -Fq '(RUNPATH)' <<< "${dynamic_section}"
grep -Fq '$ORIGIN' <<< "${dynamic_section}"
grep -Fq '/opt/fftai/fourier_dds_msgs/lib' <<< "${dynamic_section}"
if grep -Fq '(RPATH)' <<< "${dynamic_section}"; then
    echo "SDK library contains DT_RPATH instead of DT_RUNPATH." >&2
    exit 1
fi

loader_path="${sdk_prefix}/lib:${dds_prefix}/lib:${dds_prefix}/fourier_dds_msgs/lib:/usr/local/lib"
loader_output="$(LD_LIBRARY_PATH="${loader_path}" ldd "${sdk_library}")"
if grep -Fq 'not found' <<< "${loader_output}"; then
    echo "Extracted SDK library has unresolved dependencies:" >&2
    echo "${loader_output}" >&2
    exit 1
fi
core_resolution="$(grep -F 'libfourierdds_core.so =>' <<< "${loader_output}" || true)"
state_resolution="$(grep -F 'libAuroraState.so =>' <<< "${loader_output}" || true)"
if [[ "${core_resolution}" != *"=> ${dds_prefix}/"* ||
      "${state_resolution}" != *"=> ${dds_prefix}/fourier_dds_msgs/lib/"* ]]; then
    echo "SDK native dependencies did not resolve from the staged prefix." >&2
    echo "${loader_output}" >&2
    exit 1
fi

SDK_PREFIX="${sdk_prefix}" BUILD_ROOT="${build_root}" \
    "${project_root}/scripts/test_installed_headers.sh"
SDK_PREFIX="${sdk_prefix}" \
FOURIER_AURORA_SDK_DDS_PREFIX="${dds_prefix}" \
BUILD_ROOT="${build_root}" \
    "${project_root}/scripts/test_installed_cpp_sdk.sh"

echo "Debian package passed metadata, content, ELF, header, consumer, and DDS loopback checks."
