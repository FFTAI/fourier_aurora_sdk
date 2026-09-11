#!/usr/bin/env bash
set -euo pipefail

repo_root="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
dist_dir="${repo_root}/dist"
jobs="${JOBS:-8}"
arch="${ARCH:-amd64}"
case "${arch}" in
    amd64|x86_64) arch=amd64 ;;
    arm64|aarch64) arch=arm64 ;;
    *)
        echo "Unsupported ARCH for the Fourier Aurora SDK CI recipe: ${arch}." >&2
        exit 2
        ;;
esac
python_versions="${FOURIER_AURORA_SDK_PY_VERSIONS:-3.10 3.11 3.12 3.13}"
# arm64 按 thor 镜像（noble, python3.12）收敛为单 ABI；需要矩阵时用 env 覆盖。
arm_python_versions="${FOURIER_AURORA_SDK_ARM_PY_VERSIONS:-3.12}"
python_apt_repo="${FOURIER_AURORA_SDK_PYTHON_APT_REPO:-http://nexus.fftaicorp.com/repository/deadsnakes-ppa-ubuntu-jammy}"

if [[ ! -x "${repo_root}/scripts/build_deb.sh" ]]; then
    echo "Fourier Aurora SDK package scripts are missing under ${repo_root}/scripts." >&2
    exit 2
fi
if [[ "${arch}" == "amd64" && ! -x "${repo_root}/scripts/build_wheel_x86_64.sh" ]]; then
    echo "Fourier Aurora SDK package scripts are missing under ${repo_root}/scripts." >&2
    exit 2
fi
if [[ "${arch}" == "arm64" && ! -x "${repo_root}/scripts/build_wheel_arm64.sh" ]]; then
    echo "Fourier Aurora SDK package scripts are missing under ${repo_root}/scripts." >&2
    exit 2
fi

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

work_root="${FOURIER_AURORA_SDK_CI_ROOT:-/tmp/fourier-aurora-sdk-ci}"
build_root="${work_root}/build"
artifact_root="${work_root}/artifacts"
rm -rf "${work_root}" "${dist_dir}"
mkdir -p "${build_root}" "${artifact_root}/deb" "${artifact_root}/wheel" "${dist_dir}"

common_env=(
    "ARTIFACT_ROOT=${artifact_root}/deb"
    "BUILD_ROOT=${build_root}"
    "FOURIER_AURORA_SDK_COMMIT=${MODULE_HASH:-}"
    "FOURIER_AURORA_SDK_DDS_PREFIX=/opt/fftai"
    "INCLUDE_COMMIT_HASH=${INCLUDE_COMMIT_HASH:-ON}"
    "JOBS=${jobs}"
)

if [[ "${arch}" == "arm64" ]]; then
    # --- arm64 交叉流程 -----------------------------------------------------
    # deb 经 toolchain 文件交叉编译（DDS 前缀由 FIND_ROOT_PATH re-root 到
    # sysroot）；wheel 由 build_wheel_arm64.sh 直接驱动交叉编译器并按
    # linux_aarch64 手工组装（scikit-build-core 只会按宿主解释器打标签）。
    effective_sysroot="${FCB_SYSROOT:-${USER_SYSROOT:-${SYSROOT:-}}}"
    if [[ -z "${effective_sysroot}" || ! -d "${effective_sysroot}" ]]; then
        echo "arm64 cross build requires FCB_SYSROOT/USER_SYSROOT/SYSROOT pointing at the arm64 sysroot." >&2
        exit 2
    fi

    env "${common_env[@]}" "ARCH=arm64" "${repo_root}/scripts/build_deb.sh"

    mapfile -t debs < <(find "${artifact_root}/deb" -type f -name 'fourier-aurora-sdk_*.deb' -print)
    if [[ "${#debs[@]}" -ne 1 ]]; then
        echo "Expected one arm64 SDK DEB; found ${#debs[@]}." >&2
        exit 1
    fi

    env "${common_env[@]}" \
        "ARCH=arm64" \
        "ARTIFACT_ROOT=${artifact_root}/wheel" \
        "SDK_DEB=${debs[0]}" \
        "FOURIER_AURORA_SDK_ARM_PY_VERSIONS=${arm_python_versions}" \
        "${repo_root}/scripts/build_wheel_arm64.sh"

    mapfile -t wheels < <(find "${artifact_root}/wheel" -type f -name 'fourier_aurora_client-*.whl' -print)
    expected_wheel_count=0
    for python_version in ${arm_python_versions}; do
        expected_wheel_count=$((expected_wheel_count + 1))
        python_tag="cp${python_version/./}"
        matching_wheels=()
        for wheel in "${wheels[@]}"; do
            [[ "$(basename "${wheel}")" == *"-${python_tag}-${python_tag}-linux_aarch64.whl" ]] && \
                matching_wheels+=("${wheel}")
        done
        if [[ "${#matching_wheels[@]}" -ne 1 ]]; then
            echo "Expected exactly one ${python_tag} arm64 SDK wheel; found ${#matching_wheels[@]}." >&2
            exit 1
        fi
        EXPECTED_PLATFORM=linux_aarch64 \
            "${repo_root}/scripts/check_wheel_package.sh" "${matching_wheels[0]}"
    done
    if [[ "${#wheels[@]}" -ne "${expected_wheel_count}" ]]; then
        echo "Expected ${expected_wheel_count} arm64 SDK wheel(s); found ${#wheels[@]}." >&2
        exit 1
    fi
else
    # --- amd64 原生流程 -----------------------------------------------------
    install_python_matrix

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
            "${repo_root}/scripts/build_wheel_x86_64.sh"
    done

    mapfile -t debs < <(find "${artifact_root}/deb" -type f -name 'fourier-aurora-sdk_*.deb' -print)
    mapfile -t wheels < <(find "${artifact_root}/wheel" -type f -name 'fourier_aurora_client-*.whl' -print)
    expected_wheel_count=0
    for python_version in ${python_versions}; do
        expected_wheel_count=$((expected_wheel_count + 1))
        python_tag="cp${python_version/./}"
        matching_wheels=()
        for wheel in "${wheels[@]}"; do
            [[ "$(basename "${wheel}")" == *"-${python_tag}-${python_tag}-linux_x86_64.whl" ]] && \
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
fi

cp "${debs[0]}" "${wheels[@]}" "${dist_dir}/"
echo "Built ${dist_dir}/$(basename "${debs[0]}")"
for wheel in "${wheels[@]}"; do
    echo "Built ${dist_dir}/$(basename "${wheel}")"
done
