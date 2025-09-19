#!/bin/bash

# 该脚本首先会对 **idl 文件夹** 下所有文件调用 `fastddsgen`，在 **autogenerate 文件夹** 下生成python和c++相关文件。
# 然后对每个 idl 生成的项目进行编译安装到系统目录，供 c++ 程序以及 python 程序调用。

set -e  # 在任意命令出错时立即退出

# Determine sudo usage
SUDO_CMD=""
if [ ! -w "$INSTALL_DIR" ] && [ "$(id -u)" -ne 0 ]; then
    SUDO_CMD="sudo"
    echo "Note: Will use sudo for installation"
fi

# ----------------------------------------------------------------------------------------------------

IDL_ROOT="idl"
OUTPUT_ROOT="autogenerate"

# 将输出根目录转换为绝对路径
OUTPUT_ROOT=$(realpath "${OUTPUT_ROOT}")
IDL_ROOT=$(realpath "${IDL_ROOT}")

# 确保 OUTPUT_ROOT 存在
mkdir -p "${OUTPUT_ROOT}"

find "${IDL_ROOT}" -name "*.idl" | while read idl; do
    # 提取 IDL 文件名，不带扩展名
    IDL_NAME=$(basename "$idl" .idl)

    # 确定目标输出文件夹，只包含 IDL 文件名
    OUTPUT_DIR="${OUTPUT_ROOT}/${IDL_NAME}"
    OUTPUT_DIR=$(realpath "${OUTPUT_DIR}")

    # 确保目标输出文件夹存在
    mkdir -p "${OUTPUT_DIR}"

    # 检查目录是否成功创建
    if [ ! -d "${OUTPUT_DIR}" ]; then
        echo "Error: Output directory ${OUTPUT_DIR} was not created successfully."
        exit 1
    fi

    echo "Generating code for ${IDL_NAME} in ${OUTPUT_DIR}..."

    # 将 .idl 文件复制到目标目录中
    cp "${idl}" "${OUTPUT_DIR}/"

    # 切换到目标目录并运行 fastddsgen
    pushd "${OUTPUT_DIR}" > /dev/null

    # 确保 fastddsgen 知道正确的输出路径
    fastddsgen "${IDL_NAME}.idl" -replace -python -d "${OUTPUT_DIR}" -I "${IDL_ROOT}" -I "${IDL_ROOT}/std_msgs/msg" -I "${IDL_ROOT}/fourier_msgs/msg"

    if [ $? -ne 0 ]; then
        echo "Error: fastddsgen failed for ${IDL_NAME}"
        popd > /dev/null
        exit 1
    fi

    popd > /dev/null
done

# 定义安装路径
INSTALL_PREFIX="/usr/local"
BUILD_DIR="build"

# 遍历 python_msgs_generated 目录下的每一个模块目录
for MODULE_DIR in "${OUTPUT_ROOT}"/*; do
    if [ -d "$MODULE_DIR" ] && [ -f "$MODULE_DIR/CMakeLists.txt" ]; then
        # 获取模块名称
        MODULE_NAME=$(basename "$MODULE_DIR")

        echo "Building and installing module: $MODULE_NAME"

        # 创建或清理构建目录
        MODULE_BUILD_DIR="${MODULE_DIR}/${BUILD_DIR}"

        if [ -d "$MODULE_BUILD_DIR" ]; then
            echo "Cleaning existing build directory: $MODULE_BUILD_DIR"
            rm -rf "$MODULE_BUILD_DIR"
        fi

        mkdir -p "$MODULE_BUILD_DIR"

        # 查找并安装头文件
        INCLUDE_DIR="${MODULE_DIR}"

        echo "install(DIRECTORY \${CMAKE_CURRENT_SOURCE_DIR}/ DESTINATION include/fourier_dds_msgs/${MODULE_NAME} FILES_MATCHING PATTERN "*.hpp" PATTERN "build" EXCLUDE)" >> "${MODULE_DIR}/CMakeLists.txt"
        # 进入构建目录
        cd "$MODULE_BUILD_DIR"

        # 配置 CMake
        cmake .. -DCMAKE_INSTALL_PREFIX=${INSTALL_PREFIX}/fourier_dds_msgs

        # 编译
        make -j$(nproc)

        # 安装
        $SUDO_CMD make install

        # 返回上级目录
        cd ../../..
    fi
done

echo "All modules have been installed successfully."
