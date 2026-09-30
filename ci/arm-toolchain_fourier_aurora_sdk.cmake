set(CMAKE_SYSTEM_NAME Linux)
set(CMAKE_SYSTEM_PROCESSOR aarch64)

set(CMAKE_C_COMPILER aarch64-linux-gnu-gcc)
set(CMAKE_CXX_COMPILER aarch64-linux-gnu-g++)

if(DEFINED ENV{USER_SYSROOT})
    set(CMAKE_SYSROOT "$ENV{USER_SYSROOT}")
elseif(DEFINED ENV{SYSROOT})
    set(CMAKE_SYSROOT "$ENV{SYSROOT}")
endif()
# /opt/sysroot 常是指向平台 sysroot 的软链; FIND_ROOT_PATH 的 re-root 按字符串
# 匹配, 构建脚本传入的依赖前缀已 realpath —— sysroot 必须同样规范化为真实路径,
# 否则 platform 绝对路径会被误拼成 <sysroot>/opt/platform-sdk/... 导致查找失败。
if(CMAKE_SYSROOT)
    get_filename_component(_fcb_sysroot_real "${CMAKE_SYSROOT}" REALPATH)
    set(CMAKE_SYSROOT "${_fcb_sysroot_real}")
endif()

# 链内 arm64 SDK 解包在 ${CMAKE_SYSROOT}/opt/fftai(非标准布局), 必须加入搜索根。
set(CMAKE_FIND_ROOT_PATH ${CMAKE_SYSROOT} /opt/fftai)
set(CMAKE_FIND_ROOT_PATH_MODE_PROGRAM NEVER)
set(CMAKE_FIND_ROOT_PATH_MODE_LIBRARY ONLY)
set(CMAKE_FIND_ROOT_PATH_MODE_INCLUDE ONLY)
set(CMAKE_FIND_ROOT_PATH_MODE_PACKAGE ONLY)

# 链内依赖(fourier_dds 等)的 config 不在标准相对布局, 显式 <pkg>_DIR 探测
# (sysroot 解包优先, 容器根手动安装兜底), 与 cumotion/manipulate toolchain 同模式。
foreach(_pkg fourier_dds fourier_dds_msgs)
    if(NOT DEFINED ${_pkg}_DIR)
        if(EXISTS "${CMAKE_SYSROOT}/opt/fftai/lib/cmake/${_pkg}/${_pkg}Config.cmake")
            set(${_pkg}_DIR "${CMAKE_SYSROOT}/opt/fftai/lib/cmake/${_pkg}" CACHE PATH "Path to ${_pkg}Config.cmake")
        elseif(EXISTS "/opt/fftai/lib/cmake/${_pkg}/${_pkg}Config.cmake")
            set(${_pkg}_DIR "/opt/fftai/lib/cmake/${_pkg}" CACHE PATH "Path to ${_pkg}Config.cmake")
        endif()
    endif()
endforeach()
