#!/usr/bin/env bash
set -euo pipefail

repo_root="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
dist_dir="${repo_root}/dist"
jobs="${JOBS:-8}"
arch="${ARCH:-amd64}"
python_versions="${FOURIER_AURORA_SDK_PY_VERSIONS:-3.10 3.11 3.12 3.13}"
python_apt_repo="${FOURIER_AURORA_SDK_PYTHON_APT_REPO:-http://nexus.fftaicorp.com/repository/deadsnakes-ppa-ubuntu-jammy}"

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

dependency_check='
from importlib.metadata import version
expected = {
    "build": "1.4.0",
    "cmake": "3.22.1",
    "pybind11": "3.0.4",
    "scikit-build-core": "1.0.3",
}
assert all(version(name) == wanted for name, wanted in expected.items())
'

install_python_matrix() {
    local version package apt_source
    local -a missing=()
    declare -A wanted=()

    for version in ${python_versions}; do
        case "${version}" in
            3.10|3.11|3.12|3.13) ;;
            *)
                echo "Unsupported SDK wheel Python version: ${version}" >&2
                exit 2
                ;;
        esac
        command -v "python${version}" >/dev/null 2>&1 || wanted["python${version}"]=1
        [[ -f "/usr/include/python${version}/Python.h" ]] || wanted["python${version}-dev"]=1
        dpkg-query -W -f='${Status}' "python${version}-venv" 2>/dev/null | \
            grep -q 'install ok installed' || wanted["python${version}-venv"]=1
    done
    for package in "${!wanted[@]}"; do
        missing+=("${package}")
    done
    if [[ "${#missing[@]}" -gt 0 ]]; then
        export DEBIAN_FRONTEND=noninteractive
        apt_source=/etc/apt/sources.list.d/fourier-aurora-sdk-python-matrix.list
        printf 'deb [trusted=yes arch=%s] %s jammy main\n' \
            "$(dpkg --print-architecture)" "${python_apt_repo}" >"${apt_source}"
        apt-get update \
            -o "Dir::Etc::sourcelist=${apt_source}" \
            -o Dir::Etc::sourceparts=- \
            -o APT::Get::List-Cleanup=0
        apt-get install -y --no-install-recommends "${missing[@]}"
    fi
    for version in ${python_versions}; do
        command -v "python${version}" >/dev/null 2>&1 || {
            echo "python${version} installation failed" >&2
            exit 1
        }
        [[ -f "/usr/include/python${version}/Python.h" ]] || {
            echo "python${version} development headers are missing" >&2
            exit 1
        }
    done
}

if [[ "${arch}" == amd64 ]]; then
    install_python_matrix
else
    # The cross image supplies host Python and the matching target development SDK.
    command -v "python${TARGET_PYTHON_VERSION}" >/dev/null
    [[ -f "${SYSROOT}/usr/include/python${TARGET_PYTHON_VERSION}/Python.h" ]]
fi

work_root="${FOURIER_AURORA_SDK_CI_ROOT:-/tmp/fourier-aurora-sdk-ci}"
build_root="${work_root}/build"
artifact_root="${work_root}/artifacts"
rm -rf "${work_root}" "${dist_dir}"
mkdir -p "${build_root}" "${artifact_root}/deb" "${artifact_root}/wheel" "${dist_dir}"

common_env=(
    "ARTIFACT_ROOT=${artifact_root}/deb"
    "BUILD_ROOT=${build_root}"
    "FOURIER_AURORA_SDK_COMMIT=${MODULE_HASH:-}"
    "FOURIER_AURORA_SDK_DDS_PREFIX=${FOURIER_AURORA_SDK_DDS_PREFIX:-${SYSROOT:-}/opt/fftai}"
    "INCLUDE_COMMIT_HASH=${INCLUDE_COMMIT_HASH:-ON}"
    "JOBS=${jobs}"
)
env "${common_env[@]}" "${repo_root}/scripts/build_deb.sh"

common_env[0]="ARTIFACT_ROOT=${artifact_root}/wheel"
for python_version in ${python_versions}; do
    python_tag="cp${python_version/./}"
    venv_dir="${work_root}/venvs/${python_tag}"
    "python${python_version}" -m venv "${venv_dir}"
    "${venv_dir}/bin/python" -m pip install \
        --disable-pip-version-check \
        --no-compile \
        --retries "${FOURIER_AURORA_SDK_PIP_RETRIES:-2}" \
        --timeout "${FOURIER_AURORA_SDK_PIP_TIMEOUT:-120}" \
        --upgrade \
        "build==1.4.0" \
        "cmake==3.22.1" \
        "pybind11==3.0.4" \
        "scikit-build-core==1.0.3"
    "${venv_dir}/bin/python" -c "${dependency_check}"
    env "${common_env[@]}" \
        "PYTHON_BIN=${venv_dir}/bin/python" \
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

cp "${debs[0]}" "${wheels[@]}" "${dist_dir}/"
echo "Built ${dist_dir}/$(basename "${debs[0]}")"
for wheel in "${wheels[@]}"; do
    echo "Built ${dist_dir}/$(basename "${wheel}")"
done
