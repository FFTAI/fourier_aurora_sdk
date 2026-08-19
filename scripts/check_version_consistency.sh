#!/usr/bin/env bash
set -euo pipefail

project_root="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
version="$(tr -d '\n' < "${project_root}/VERSION")"

if [[ ! "${version}" =~ ^[0-9]+\.[0-9]+\.[0-9]+$ ]]; then
    echo "VERSION must contain exactly one major.minor.patch value." >&2
    exit 1
fi
if [[ "$(wc -l < "${project_root}/VERSION")" -ne 1 ]]; then
    echo "VERSION must be a single line terminated by a newline." >&2
    exit 1
fi

grep -Fq 'dynamic = ["version"]' "${project_root}/pyproject.toml"
grep -Fq 'provider = "scikit_build_core.metadata.regex"' "${project_root}/pyproject.toml"
grep -Fq 'input = "VERSION"' "${project_root}/pyproject.toml"
version_major="${version%%.*}"
grep -Eq "set\\(FOURIER_AURORA_SDK_SOVERSION[[:space:]]+${version_major}\\)" \
    "${project_root}/CMakeLists.txt"
grep -Fq "### Version ${version} (unreleased)" "${project_root}/CHANGELOG"

build_root="${BUILD_ROOT:-/tmp}"
mkdir -p "${build_root}"
build_dir="$(mktemp -d "${build_root%/}/fourier-aurora-sdk-version-check.XXXXXX")"
trap 'rm -rf "${build_dir}"' EXIT

cmake -S "${project_root}" -B "${build_dir}" \
    -DFOURIER_AURORA_SDK_ENABLE_DDS_TRANSPORT=OFF \
    -DFOURIER_AURORA_SDK_BUILD_PYTHON=OFF \
    -DFOURIER_AURORA_SDK_BUILD_TESTS=OFF \
    -DFOURIER_AURORA_SDK_BUILD_CLIENT_TOOLS=OFF \
    -DFOURIER_AURORA_SDK_BUILD_EXAMPLES=OFF >/dev/null

grep -Fq "CMAKE_PROJECT_VERSION:STATIC=${version}" "${build_dir}/CMakeCache.txt"
grep -Fq "set(CPACK_PACKAGE_VERSION \"${version}\")" "${build_dir}/CPackConfig.cmake"

echo "Version metadata is consistent at ${version}."
