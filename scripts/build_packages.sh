#!/usr/bin/env bash
set -euo pipefail

repo_root="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
dist_dir="${OUTPUT_DIR:-${repo_root}/dist}"
jobs="${JOBS:-8}"
arch="${ARCH:-amd64}"
python_versions="${FOURIER_AURORA_SDK_PY_VERSIONS:-3.10 3.11 3.12 3.13}"

case "${arch}" in
    amd64|x86_64) arch=amd64; wheel_arch=x86_64 ;;
    arm64|aarch64)
        arch=arm64; wheel_arch=aarch64
        : "${SYSROOT:?ARM packaging requires the target SYSROOT}"
        export SYSROOT="$(realpath -e "${SYSROOT}")"
        : "${CMAKE_TOOLCHAIN_FILE:?ARM packaging requires the image toolchain}"
        : "${TARGET_PYTHON_VERSION:?ARM packaging requires the image Python version}"
        # Build only ABIs supplied by this target SDK, never host-only Python headers.
        python_versions="${FOURIER_AURORA_SDK_PY_VERSIONS:-${TARGET_PYTHON_VERSION}}"
        for version in ${python_versions}; do
            [[ "${version}" == "${TARGET_PYTHON_VERSION}" ]] || {
                echo "ARM image provides Python ${TARGET_PYTHON_VERSION}, not ${version}" >&2; exit 2;
            }
        done
        ;;
    *) echo "Unsupported SDK architecture: ${arch}" >&2; exit 2 ;;
esac
wheel_script="${repo_root}/scripts/build_wheel_${arch/amd64/x86_64}.sh"
[[ -x "${repo_root}/scripts/build_deb.sh" && -x "${wheel_script}" ]] || {
    echo "SDK package scripts are missing" >&2; exit 2;
}

check_python_matrix() {
    local version
    [[ -n "${python_versions//[[:space:]]/}" ]] || {
        echo "Select at least one Python version with FOURIER_AURORA_SDK_PY_VERSIONS." >&2
        exit 2
    }
    declare -A seen=()
    for version in ${python_versions}; do
        case "${version}" in
            3.10|3.11|3.12|3.13) ;;
            *)
                echo "Unsupported SDK wheel Python version: ${version}" >&2
                exit 2
                ;;
        esac
        [[ -z "${seen[${version}]:-}" ]] || {
            echo "Duplicate Python version: ${version}" >&2
            exit 2
        }
        seen["${version}"]=1
        command -v "python${version}" >/dev/null 2>&1 || {
            echo "Prepare python${version} on PATH before building packages." >&2
            exit 2
        }
        "python${version}" -c 'import build, pybind11, scikit_build_core' || {
            echo "Install build, pybind11>=2.13.6 and scikit-build-core>=0.9.10 in python${version} first." >&2
            exit 2
        }
        if [[ "${arch}" == amd64 ]]; then
            "python${version}" -c 'import pathlib, sysconfig; assert (pathlib.Path(sysconfig.get_path("include")) / "Python.h").is_file(), "Python development headers are missing"'
        fi
    done
}

check_python_matrix
if [[ "${arch}" == arm64 ]]; then
    [[ -f "${SYSROOT}/usr/include/python${TARGET_PYTHON_VERSION}/Python.h" ]]
fi

mkdir -p "${BUILD_ROOT:-/tmp}"
work_root="$(mktemp -d "${BUILD_ROOT:-/tmp}/fourier-aurora-sdk-packages.XXXXXX")"
build_root="${work_root}/build"
artifact_root="${work_root}/artifacts"
mkdir -p "${build_root}" "${artifact_root}/deb" "${artifact_root}/wheel" "${dist_dir}"

# Match the toolchain's canonical sysroot so CMake
# does not prepend that root again to an already staged dependency path.
dds_prefix="$(realpath -e "${FOURIER_AURORA_SDK_DDS_PREFIX:-${SYSROOT:-}/opt/fftai}")"

version="$(tr -d '[:space:]' < "${repo_root}/VERSION")"
for python_version in ${python_versions}; do
    "python${python_version}" -m build "${repo_root}" --sdist --no-isolation \
        --outdir "${artifact_root}/source"
    source_package="${artifact_root}/source/fourier_aurora_client-${version}.tar.gz"
    "python${python_version}" "${repo_root}/scripts/check_source_package.py" "${source_package}"
    break
done

common_env=(
    "ARTIFACT_ROOT=${artifact_root}/deb"
    "BUILD_ROOT=${build_root}"
    "FOURIER_AURORA_SDK_COMMIT=${FOURIER_AURORA_SDK_COMMIT:-}"
    "FOURIER_AURORA_SDK_DDS_PREFIX=${dds_prefix}"
    "INCLUDE_COMMIT_HASH=${INCLUDE_COMMIT_HASH:-OFF}"
    "JOBS=${jobs}"
)
env "${common_env[@]}" "${repo_root}/scripts/build_deb.sh"

common_env[0]="ARTIFACT_ROOT=${artifact_root}/wheel"
for python_version in ${python_versions}; do
    env "${common_env[@]}" \
        "PYTHON_BIN=$(command -v "python${python_version}")" \
        "${wheel_script}"
done

mapfile -t debs < <(find "${artifact_root}/deb" -type f -name 'fourier-aurora-sdk_*.deb' -print)
mapfile -t wheels < <(find "${artifact_root}/wheel" -type f -name 'fourier_aurora_client-*.whl' -print)
expected_wheel_count=0
for python_version in ${python_versions}; do
    expected_wheel_count=$((expected_wheel_count + 1))
    python_tag="cp${python_version/./}"
    matching_wheels=()
    for wheel in "${wheels[@]}"; do
        [[ "$(basename "${wheel}")" == *"-${python_tag}-${python_tag}-linux_${wheel_arch}.whl" ]] && \
            matching_wheels+=("${wheel}")
    done
    if [[ "${#matching_wheels[@]}" -ne 1 ]]; then
        echo "Expected exactly one ${python_tag} SDK wheel; found ${#matching_wheels[@]}." >&2
        exit 1
    fi
    "${repo_root}/scripts/check_wheel_package.sh" "${matching_wheels[0]}"
done
if [[ "${#debs[@]}" -ne 1 || "${#wheels[@]}" -ne "${expected_wheel_count}" ]]; then
    echo "Expected one SDK DEB and ${expected_wheel_count} SDK wheels; found ${#debs[@]} DEB(s) and ${#wheels[@]} wheel(s)." >&2
    exit 1
fi

cp "${source_package}" "${debs[0]}" "${wheels[@]}" "${dist_dir}/"
echo "Built ${dist_dir}/$(basename "${source_package}")"
echo "Built ${dist_dir}/$(basename "${debs[0]}")"
for wheel in "${wheels[@]}"; do
    echo "Built ${dist_dir}/$(basename "${wheel}")"
done
