#!/usr/bin/env bash
set -euo pipefail

if [[ $# -ne 1 ]]; then
    echo "Usage: $0 <debug|release|asan-ubsan|tsan>" >&2
    exit 2
fi

profile="$1"
project_root="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
jobs="${JOBS:-$(nproc)}"
test_tmp_root="${TEST_TMP_ROOT:-/tmp}"

for legacy_override in ENABLE_DDS RUN_PYTHON_TESTS BUILD_ROOT; do
    if [[ -n "${!legacy_override+x}" ]]; then
        echo "${legacy_override} cannot override the fixed ${profile} profile." >&2
        exit 2
    fi
done

require_command() {
    if ! command -v "$1" >/dev/null 2>&1; then
        echo "Required command is unavailable for ${profile}: $1" >&2
        exit 1
    fi
}

require_command cmake
require_command ctest
require_command "${CXX:-c++}"

cmake_args=()
case "${profile}" in
    debug)
        build_type="Debug"
        build_python="ON"
        ;;
    release)
        build_type="Release"
        build_python="ON"
        ;;
    asan-ubsan)
        build_type="RelWithDebInfo"
        build_python="OFF"
        sanitizer_flags="-fsanitize=address,undefined -fno-omit-frame-pointer"
        cmake_args+=(
            "-DCMAKE_CXX_FLAGS=${sanitizer_flags}"
            "-DCMAKE_EXE_LINKER_FLAGS=${sanitizer_flags}"
            "-DCMAKE_SHARED_LINKER_FLAGS=${sanitizer_flags}"
        )
        export ASAN_OPTIONS="${ASAN_OPTIONS:-detect_leaks=0:halt_on_error=1}"
        export UBSAN_OPTIONS="${UBSAN_OPTIONS:-print_stacktrace=1:halt_on_error=1}"
        ;;
    tsan)
        build_type="RelWithDebInfo"
        build_python="OFF"
        sanitizer_flags="-fsanitize=thread -fno-omit-frame-pointer"
        cmake_args+=(
            "-DCMAKE_CXX_FLAGS=${sanitizer_flags}"
            "-DCMAKE_EXE_LINKER_FLAGS=${sanitizer_flags}"
            "-DCMAKE_SHARED_LINKER_FLAGS=${sanitizer_flags}"
        )
        export TSAN_OPTIONS="${TSAN_OPTIONS:-halt_on_error=1:second_deadlock_stack=1}"
        require_command setarch
        ;;
    *)
        echo "Unknown test profile: ${profile}" >&2
        exit 2
        ;;
esac

if [[ "${build_python}" == "ON" ]]; then
    require_command python3
    python3 -c 'import pybind11, pytest' 2>/dev/null || {
        echo "${profile} requires Python 3, pybind11, and pytest." >&2
        exit 1
    }
fi

mkdir -p "${test_tmp_root}"
build_dir="$(mktemp -d "${test_tmp_root%/}/fourier_aurora_sdk_${profile}.XXXXXX")"

echo "Aurora SDK test profile: ${profile}"
echo "  build_dir=${build_dir}"
echo "  build_type=${build_type}"
echo "  tests=ON"
echo "  python=${build_python}"
echo "  dds=OFF"
echo "  client_tools=OFF"
echo "  examples=OFF"

cmake -S "${project_root}" -B "${build_dir}" \
    -DCMAKE_BUILD_TYPE="${build_type}" \
    -DFOURIER_AURORA_SDK_BUILD_TESTS=ON \
    -DFOURIER_AURORA_SDK_BUILD_CLIENT_TOOLS=OFF \
    -DFOURIER_AURORA_SDK_BUILD_EXAMPLES=OFF \
    -DFOURIER_AURORA_SDK_BUILD_PYTHON="${build_python}" \
    -DFOURIER_AURORA_SDK_ENABLE_DDS_TRANSPORT=OFF \
    "${cmake_args[@]}"

cmake --build "${build_dir}" --parallel "${jobs}"

ctest_command=(ctest --test-dir "${build_dir}" --output-on-failure)
case "${profile}" in
    asan-ubsan)
        asan_runtime="$(${CXX:-c++} -print-file-name=libasan.so)"
        if [[ ! -f "${asan_runtime}" ]]; then
            echo "AddressSanitizer runtime not found: ${asan_runtime}" >&2
            exit 1
        fi
        env LD_PRELOAD="${asan_runtime}${LD_PRELOAD:+:${LD_PRELOAD}}" \
            "${ctest_command[@]}"
        ;;
    tsan)
        setarch "$(uname -m)" -R "${ctest_command[@]}"
        ;;
    *)
        "${ctest_command[@]}"
        ;;
esac

if [[ "${build_python}" == "ON" ]]; then
    PYTHONPATH="${build_dir}/python" \
        PYTHONPYCACHEPREFIX="${build_dir}/pycache" \
        python3 -m pytest -q "${project_root}/tests/python"
fi

echo "Passed ${profile}; build retained at ${build_dir}"
