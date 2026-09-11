#!/usr/bin/env bash
# arm64 交叉 wheel 构建（FCB GR3THOR 门禁用）。
#
# 形态与 fourierddspy 的 arm64 流程同型：scikit-build-core 只会按宿主解释器
# 打标签，交叉产物必须手工组装——直接以 aarch64-linux-gnu-g++ 编译 pybind11
# 扩展（绑定只需公共头，见 python/bindings），从已构建的 arm64 deb 中取出带
# 干净 INSTALL_RPATH 的 libfourier_aurora_sdk.so.<soversion> 一并封入 wheel，
# dist-info 由 scripts/assemble_wheel.py 生成。
#
# 目标侧 CPython 头文件按需从 apt-ports 代理拉取 libpython<X.Y>-dev:arm64
# 并解入 sysroot（幂等；arm64 架构与 ports 源由 FCB ensure_arm_host_sources
# 在模块构建入口前配好）。扩展在 Linux 上不链 libpython，符号在 import 时
# 由解释器解析，因此仅需头文件。
set -euo pipefail

project_root="$(cd "$(dirname "$0")/.." && pwd)"
include_commit="${INCLUDE_COMMIT_HASH:-ON}"
sysroot="${FCB_SYSROOT:-${USER_SYSROOT:-${SYSROOT:-}}}"
python_versions="${FOURIER_AURORA_SDK_ARM_PY_VERSIONS:-3.12}"
sdk_deb="${SDK_DEB:-}"

require_external_root() {
    local label="$1"
    local root
    root="$(realpath -m "$2")"
    if [[ "${root}" == "${project_root}" || "${root}" == "${project_root}/"* ]]; then
        echo "${label} must be outside the SDK source tree: ${root}" >&2
        exit 2
    fi
}

if [[ -z "${sysroot}" || ! -d "${sysroot}" ]]; then
    echo "arm64 wheel build requires FCB_SYSROOT/USER_SYSROOT/SYSROOT pointing at the arm64 sysroot." >&2
    exit 2
fi
sysroot="${sysroot%/}"
if [[ -z "${sdk_deb}" || ! -f "${sdk_deb}" ]]; then
    echo "Set SDK_DEB to the built arm64 fourier-aurora-sdk Debian package." >&2
    exit 2
fi
if [[ "$(dpkg-deb -f "${sdk_deb}" Architecture)" != "arm64" ]]; then
    echo "SDK_DEB is not an arm64 package: ${sdk_deb}" >&2
    exit 2
fi

cross_cxx="${CXX:-aarch64-linux-gnu-g++}"
if ! command -v "${cross_cxx}" >/dev/null 2>&1; then
    echo "Cross compiler is unavailable: ${cross_cxx}" >&2
    exit 2
fi
command -v python3 >/dev/null 2>&1 || { echo "Host python3 is required." >&2; exit 2; }

case "${include_commit}" in
    ON)
        commit="$("${project_root}/scripts/resolve_commit_hash.sh")"
        build_tag="1g${commit}"
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

version="$(tr -d '[:space:]' < "${project_root}/VERSION")"
soversion="$(sed -nE 's/^[[:space:]]*set\(FOURIER_AURORA_SDK_SOVERSION[[:space:]]+([0-9]+)\).*$/\1/p' "${project_root}/CMakeLists.txt")"
if [[ -z "${soversion}" ]]; then
    echo "Unable to read FOURIER_AURORA_SDK_SOVERSION." >&2
    exit 1
fi

build_root="${BUILD_ROOT:-/tmp}"
artifact_root="${ARTIFACT_ROOT:-/tmp}"
require_external_root BUILD_ROOT "${build_root}"
require_external_root ARTIFACT_ROOT "${artifact_root}"
mkdir -p "${build_root}" "${artifact_root}"
work_dir="$(mktemp -d "${build_root%/}/fourier-aurora-sdk-wheel-arm64.XXXXXX")"
artifact_dir="$(mktemp -d "${artifact_root%/}/fourier-aurora-sdk-wheel-arm64-artifacts.XXXXXX")"

# --- 1. 目标侧 CPython 头文件（幂等）---------------------------------------
stage_target_python_headers() {
    local version="$1"
    local include_dir="${sysroot}/usr/include/python${version}"
    local pyconfig="${sysroot}/usr/include/aarch64-linux-gnu/python${version}/pyconfig.h"
    if [[ -f "${include_dir}/Python.h" && -f "${pyconfig}" ]]; then
        return 0
    fi
    local download_dir="${work_dir}/python-dev-${version}"
    mkdir -p "${download_dir}"
    (
        cd "${download_dir}"
        apt-get download "libpython${version}-dev:arm64"
    )
    local deb
    deb="$(find "${download_dir}" -maxdepth 1 -name "libpython${version}-dev_*_arm64.deb" -print -quit)"
    if [[ -z "${deb}" ]]; then
        echo "libpython${version}-dev:arm64 download failed." >&2
        exit 1
    fi
    dpkg-deb -x "${deb}" "${sysroot}"
    echo "Staged $(basename "${deb}") into ${sysroot}"
    if [[ ! -f "${include_dir}/Python.h" || ! -f "${pyconfig}" ]]; then
        echo "Target python${version} headers were not staged correctly." >&2
        exit 1
    fi
}

# --- 2. host 侧 pybind11 头（纯头文件，架构无关）-----------------------------
if ! python3 -c 'import pybind11; assert tuple(map(int, pybind11.__version__.split(".")[:2])) >= (2, 13)' >/dev/null 2>&1; then
    pip_args=(--upgrade 'pybind11>=2.13.6,<3')
    if python3 -m pip install --help 2>&1 | grep -q -- '--break-system-packages'; then
        pip_args=(--break-system-packages "${pip_args[@]}")
    fi
    python3 -m pip install "${pip_args[@]}"
fi
pybind_include="$(python3 -c 'import pybind11; print(pybind11.get_include())')"

# --- 3. 从 deb 取 vendored SDK 库（INSTALL_RPATH 已洗净构建树路径）-----------
deb_extract="${work_dir}/deb-root"
mkdir -p "${deb_extract}"
dpkg-deb -x "${sdk_deb}" "${deb_extract}"
sdk_lib_dir="${deb_extract}/opt/fftai/fourier_aurora_sdk/lib"
sdk_lib="${sdk_lib_dir}/libfourier_aurora_sdk.so.${soversion}"
if [[ ! -e "${sdk_lib}" || ! -L "${sdk_lib_dir}/libfourier_aurora_sdk.so" ]]; then
    echo "SDK Debian package lacks libfourier_aurora_sdk.so.${soversion}." >&2
    exit 1
fi

# --- 4. 逐版本编译扩展并组装 wheel ------------------------------------------
for python_version in ${python_versions}; do
    case "${python_version}" in
        3.10|3.11|3.12|3.13) ;;
        *)
            echo "Unsupported SDK arm64 wheel Python version: ${python_version}" >&2
            exit 2
            ;;
    esac
    stage_target_python_headers "${python_version}"
    python_tag="cp${python_version/./}"
    extension_name="_fourier_aurora_sdk.cpython-${python_tag#cp}-aarch64-linux-gnu.so"
    extension="${work_dir}/${extension_name}"

    "${cross_cxx}" -O2 -DNDEBUG -fPIC -shared -std=c++17 -fvisibility=hidden \
        --sysroot="${sysroot}" \
        -I"${project_root}/include" \
        -I"${pybind_include}" \
        -I"${sysroot}/usr/include/python${python_version}" \
        -I"${sysroot}/usr/include/aarch64-linux-gnu" \
        "${project_root}/python/bindings/module.cpp" \
        "${project_root}/python/bindings/bind_client.cpp" \
        "${project_root}/python/bindings/bind_types.cpp" \
        -L"${sdk_lib_dir}" -lfourier_aurora_sdk \
        -L"${sysroot}/opt/fftai/lib" \
        -L"${sysroot}/opt/fftai/fourier_dds_msgs/lib" \
        -Wl,--enable-new-dtags,-rpath,'$ORIGIN' \
        -Wl,-rpath-link,"${sysroot}/opt/fftai/lib" \
        -Wl,-rpath-link,"${sysroot}/opt/fftai/fourier_dds_msgs/lib" \
        -Wl,-rpath-link,"${sysroot}/usr/local/lib" \
        -o "${extension}"

    # ELF 契约：目标架构、依赖 SDK 库、RUNPATH（拒绝 RPATH）、无构建树路径。
    readelf -h "${extension}" | grep -q 'Machine:.*AArch64' || {
        echo "${extension_name} is not an AArch64 object." >&2
        exit 1
    }
    extension_dynamic="$(readelf -d "${extension}")"
    grep -Fq '(RUNPATH)' <<< "${extension_dynamic}" || {
        echo "${extension_name} lacks DT_RUNPATH." >&2
        exit 1
    }
    grep -Fq '$ORIGIN' <<< "${extension_dynamic}" || {
        echo "${extension_name} RUNPATH misses \$ORIGIN." >&2
        exit 1
    }
    grep -Fq "Shared library: [libfourier_aurora_sdk.so.${soversion}]" <<< "${extension_dynamic}" || {
        echo "${extension_name} does not depend on libfourier_aurora_sdk.so.${soversion}." >&2
        exit 1
    }
    if grep -Eq '/tmp/|/workspace/' <<< "${extension_dynamic}"; then
        echo "${extension_name} contains a build-tree path." >&2
        exit 1
    fi

    staged_root="${work_dir}/staged-${python_tag}"
    mkdir -p "${staged_root}/fourier_aurora_client"
    cp "${project_root}/python/fourier_aurora_client/__init__.py" \
       "${project_root}/python/fourier_aurora_client/client.py" \
       "${staged_root}/fourier_aurora_client/"
    cp "${extension}" "${staged_root}/fourier_aurora_client/${extension_name}"
    cp -L "${sdk_lib}" "${staged_root}/fourier_aurora_client/libfourier_aurora_sdk.so.${soversion}"

    assemble_args=(
        --package-dir "${staged_root}"
        --version "${version}"
        --tag "${python_tag}-${python_tag}-linux_aarch64"
        --license "${project_root}/LICENSE"
        --outdir "${artifact_dir}"
    )
    if [[ -n "${build_tag}" ]]; then
        assemble_args+=(--build-tag "${build_tag}")
    fi
    python3 "${project_root}/scripts/assemble_wheel.py" "${assemble_args[@]}"

    if [[ -n "${build_tag}" ]]; then
        expected_name="fourier_aurora_client-${version}-${build_tag}-${python_tag}-${python_tag}-linux_aarch64.whl"
    else
        expected_name="fourier_aurora_client-${version}-${python_tag}-${python_tag}-linux_aarch64.whl"
    fi
    if [[ ! -f "${artifact_dir}/${expected_name}" ]]; then
        echo "Expected wheel ${expected_name} was not produced." >&2
        exit 1
    fi
    echo "Built ${artifact_dir}/${expected_name}"
done

echo "Build directory: ${work_dir}"
