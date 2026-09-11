# ============================================================================
# aarch64 交叉编译工具链文件 (Fourier Aurora SDK)
# ============================================================================
# 与 auroramotioncontrol 的 toolchains/aarch64-linux-gnu.cmake 同型（该文件已在
# FCB GR3THOR 交叉容器实证，#243）：编译器按三元组发现，库/头/CMake 包一律从
# sysroot 取，可执行程序只从 host 找。host 的 /opt/fftai 等路径在
# FIND_ROOT_PATH 模式下会被自动 re-root 到 sysroot，主 CMakeLists 无需改动。
#
# 用法: USER_SYSROOT=/path/to/sysroot \
#       cmake -S . -B build -DCMAKE_TOOLCHAIN_FILE=toolchains/aarch64-linux-gnu.cmake
#
# 环境变量: TOOLCHAIN_PATH     交叉编译器所在目录（可选，缺省在 PATH 中按三元组找）
#           TOOLCHAIN_TRIPLET  编译器三元组，默认 aarch64-linux-gnu
#           USER_SYSROOT       aarch64 根文件系统（FCB 注入；缺省依次尝试 SYSROOT
#                              环境变量、/sysroot、/root/sysroot）
# ============================================================================

set(CMAKE_SYSTEM_NAME Linux)
set(CMAKE_SYSTEM_PROCESSOR aarch64)

# --------------------------------------------------------------------------
# 编译器发现: 优先 TOOLCHAIN_PATH 下的显式编译器，其次在 PATH 中按三元组候选查找。
# --------------------------------------------------------------------------
set(TOOLCHAIN_PATH "$ENV{TOOLCHAIN_PATH}")
if (DEFINED ENV{TOOLCHAIN_TRIPLET} AND NOT "$ENV{TOOLCHAIN_TRIPLET}" STREQUAL "")
  set(TOOLCHAIN_TRIPLET "$ENV{TOOLCHAIN_TRIPLET}")
else ()
  set(TOOLCHAIN_TRIPLET aarch64-linux-gnu)
endif ()

set(_triplet_candidates "${TOOLCHAIN_TRIPLET}" "aarch64-linux-gnu" "aarch64-none-linux-gnu"
                        "aarch64-buildroot-linux-gnu")

set(_compiler_found FALSE)
foreach (_triplet IN LISTS _triplet_candidates)
  if (TOOLCHAIN_PATH AND EXISTS "${TOOLCHAIN_PATH}/${_triplet}-gcc"
      AND EXISTS "${TOOLCHAIN_PATH}/${_triplet}-g++")
    set(CMAKE_C_COMPILER "${TOOLCHAIN_PATH}/${_triplet}-gcc")
    set(CMAKE_CXX_COMPILER "${TOOLCHAIN_PATH}/${_triplet}-g++")
    set(TOOLCHAIN_TRIPLET "${_triplet}")
    set(_compiler_found TRUE)
    break()
  endif ()
  find_program(_cc NAMES "${_triplet}-gcc")
  find_program(_cxx NAMES "${_triplet}-g++")
  if (_cc AND _cxx)
    set(CMAKE_C_COMPILER "${_cc}")
    set(CMAKE_CXX_COMPILER "${_cxx}")
    set(TOOLCHAIN_TRIPLET "${_triplet}")
    get_filename_component(TOOLCHAIN_PATH "${_cxx}" DIRECTORY)
    set(_compiler_found TRUE)
    break()
  endif ()
endforeach ()

if (NOT _compiler_found)
  message(
    FATAL_ERROR "Cannot find aarch64 cross compiler. Tried TOOLCHAIN_PATH='${TOOLCHAIN_PATH}' "
                "and triplets: ${_triplet_candidates}")
endif ()

# --------------------------------------------------------------------------
# sysroot 发现: USER_SYSROOT (变量/环境) -> SYSROOT 环境变量 -> /sysroot -> /root/sysroot。
# --------------------------------------------------------------------------
if (DEFINED USER_SYSROOT)
  set(_FOURIER_AURORA_SDK_SYSROOT "${USER_SYSROOT}")
elseif (DEFINED ENV{USER_SYSROOT})
  set(_FOURIER_AURORA_SDK_SYSROOT "$ENV{USER_SYSROOT}")
elseif (DEFINED ENV{SYSROOT})
  set(_FOURIER_AURORA_SDK_SYSROOT "$ENV{SYSROOT}")
elseif (EXISTS "/sysroot")
  set(_FOURIER_AURORA_SDK_SYSROOT "/sysroot")
elseif (EXISTS "/root/sysroot")
  set(_FOURIER_AURORA_SDK_SYSROOT "/root/sysroot")
else ()
  set(_FOURIER_AURORA_SDK_SYSROOT "")
endif ()
string(REGEX REPLACE "/$" "" _FOURIER_AURORA_SDK_SYSROOT "${_FOURIER_AURORA_SDK_SYSROOT}")
set(USER_SYSROOT "${_FOURIER_AURORA_SDK_SYSROOT}"
    CACHE PATH "Sysroot used by the Fourier Aurora SDK cross build")

if (USER_SYSROOT)
  set(CMAKE_SYSROOT "${USER_SYSROOT}")
  set(CMAKE_FIND_ROOT_PATH "${USER_SYSROOT}")
endif ()

# 库/头文件/包一律从 sysroot 取；可执行程序只从 host 找，交叉产物无法在 host 运行。
set(CMAKE_FIND_ROOT_PATH_MODE_PROGRAM NEVER)
set(CMAKE_FIND_ROOT_PATH_MODE_LIBRARY ONLY)
set(CMAKE_FIND_ROOT_PATH_MODE_INCLUDE ONLY)
set(CMAKE_FIND_ROOT_PATH_MODE_PACKAGE ONLY)
set(CMAKE_LINK_DEPENDS_NO_SHARED ON)

# --------------------------------------------------------------------------
# sysroot 编译/链接选项。
# --------------------------------------------------------------------------
if (USER_SYSROOT)
  add_compile_options(--sysroot=${CMAKE_SYSROOT} -I${USER_SYSROOT}/usr/include/aarch64-linux-gnu
                      -I${USER_SYSROOT}/usr/local/include -Wall -O2)

  add_link_options(--sysroot=${CMAKE_SYSROOT} -L${USER_SYSROOT}/usr/local/lib
                   -L${USER_SYSROOT}/usr/local/lib/aarch64-linux-gnu)
endif ()
