#!/usr/bin/env bash
set -euo pipefail

if [[ $# -ne 1 ]]; then
    echo "Usage: $0 <debug|release|asan-ubsan|tsan>" >&2
    exit 2
fi

profile="$1"
project_root="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
build_root="${BUILD_ROOT:-${project_root}/build_tests}"
build_dir="${build_root}/${profile}"
jobs="${JOBS:-$(nproc)}"
enable_dds="${ENABLE_DDS:-OFF}"
run_python_tests="${RUN_PYTHON_TESTS:-ON}"
cmake_args=()

case "${profile}" in
    debug)
        build_type="Debug"
        ;;
    release)
        build_type="Release"
        ;;
    asan-ubsan)
        build_type="RelWithDebInfo"
        sanitizer_flags="-fsanitize=address,undefined -fno-omit-frame-pointer"
        cmake_args+=(
            "-DCMAKE_CXX_FLAGS=${sanitizer_flags}"
            "-DCMAKE_EXE_LINKER_FLAGS=${sanitizer_flags}"
            "-DCMAKE_SHARED_LINKER_FLAGS=${sanitizer_flags}"
        )
        export ASAN_OPTIONS="${ASAN_OPTIONS:-detect_leaks=0:halt_on_error=1}"
        export UBSAN_OPTIONS="${UBSAN_OPTIONS:-print_stacktrace=1:halt_on_error=1}"
        run_python_tests="${RUN_PYTHON_TESTS:-OFF}"
        ;;
    tsan)
        build_type="RelWithDebInfo"
        sanitizer_flags="-fsanitize=thread -fno-omit-frame-pointer"
        cmake_args+=(
            "-DCMAKE_CXX_FLAGS=${sanitizer_flags}"
            "-DCMAKE_EXE_LINKER_FLAGS=${sanitizer_flags}"
            "-DCMAKE_SHARED_LINKER_FLAGS=${sanitizer_flags}"
        )
        export TSAN_OPTIONS="${TSAN_OPTIONS:-halt_on_error=1:second_deadlock_stack=1}"
        run_python_tests="${RUN_PYTHON_TESTS:-OFF}"
        ;;
    *)
        echo "Unknown test profile: ${profile}" >&2
        exit 2
        ;;
esac

cmake -S "${project_root}" -B "${build_dir}" \
    -DCMAKE_BUILD_TYPE="${build_type}" \
    -DFOURIER_AURORA_SDK_BUILD_TESTS=ON \
    -DFOURIER_AURORA_SDK_BUILD_EXAMPLES=OFF \
    -DFOURIER_AURORA_SDK_BUILD_PYTHON="${run_python_tests}" \
    -DFOURIER_AURORA_SDK_ENABLE_DDS_TRANSPORT="${enable_dds}" \
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
        env LD_PRELOAD="${asan_runtime}${LD_PRELOAD:+:${LD_PRELOAD}}" "${ctest_command[@]}"
        ;;
    tsan)
        setarch "$(uname -m)" -R "${ctest_command[@]}"
        ;;
    *)
        "${ctest_command[@]}"
        ;;
esac

if [[ "${run_python_tests}" == "ON" ]] &&
   [[ -d "${project_root}/tests/python" ]] &&
   find "${project_root}/tests/python" -type f -name 'test_*.py' -print -quit | grep -q .; then
    PYTHONPATH="${build_dir}:${project_root}/python${PYTHONPATH:+:${PYTHONPATH}}" \
        python3 -m pytest "${project_root}/tests/python"
fi
