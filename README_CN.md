# Fourier Aurora SDK

本仓库是重构后的 Aurora SDK：

- C++ SDK 位于 `include/fourier_aurora_sdk` 和 `src/`
- Python SDK 包名为 `fourier_aurora_client`，通过 pybind11 绑定 C++ SDK
- DDS 通信使用 `fourierdds`

公开 API 只暴露 SDK 自定义类型。生成的 DDS 消息类型和原始 `fourierdds` 对象只作为内部实现细节使用。

## 依赖

Stage 1 使用内部系统包。构建前需要准备：

- `fourierdds`
- 生成后的 `fourier_dds_msgs`
- 与消息库匹配的 FastDDS/FastCDR
- 构建 Python 绑定时需要 pybind11

已验证的内部构建环境：

```bash
docker.fftaicorp.com/base/aurora_medium:latest
```

## 构建

仅构建 C++：

```bash
cmake -S . -B build -DFOURIER_AURORA_SDK_BUILD_PYTHON=OFF
cmake --build build -j4
ctest --test-dir build --output-on-failure
```

启用 DDS transport 并构建 Python 绑定：

```bash
cmake -S . -B build \
  -DCMAKE_PREFIX_PATH=/opt/fftai \
  -DFOURIER_AURORA_SDK_BUILD_PYTHON=ON \
  -DFOURIER_AURORA_SDK_ENABLE_DDS_TRANSPORT=ON
cmake --build build -j4
ctest --test-dir build --output-on-failure
```

构建 C++ SDK Debian 包：

```bash
bash scripts/build_deb.sh
sudo dpkg -i build_deb/fourier-aurora-sdk_*.deb
```

Debian 包默认安装到：

```text
/opt/fftai/fourier_aurora_sdk
```

同时会安装：

- `/etc/ld.so.conf.d/fourier_aurora_sdk.conf`，用于运行时库查找

直接用 `g++` 编译并运行已安装 SDK 的 smoke test：

```bash
bash scripts/test_installed_cpp_sdk.sh
```

构建 Python wheel：

```bash
python3 -m build --wheel
pip install dist/fourier_aurora_client-*.whl
```

Python wheel 会把 Aurora C++ SDK 动态库和 pybind 模块一起打包，因此仅使用
Python SDK 时不需要额外安装 C++ SDK 的 Debian 包。Stage 1 仍然要求系统中已安装
原生 DDS 依赖：

- `/opt/fftai/fourier_dds_msgs/lib` 下的 `fourier_dds_msgs` 生成库
- `/usr/local/lib` 下的 FastDDS/FastCDR
- 构建时可用的 `fourierdds` 头文件和库

wheel 构建会启用 DDS transport，但只构建 Python 模块目标，不构建 C++ 示例。

安装后的 C++ 使用示例会复制到：

```text
/opt/fftai/fourier_aurora_sdk/share/doc/fourier_aurora_sdk/examples/cpp/usage
```

使用 `FOURIER_AURORA_SDK_ENABLE_DDS_TRANSPORT=ON` 构建时，示例可执行文件会安装到：

```text
/opt/fftai/fourier_aurora_sdk/bin/fourier_aurora_sdk_examples
```

它们支持可选参数 `[domain_id] [user_id]`：

```bash
/opt/fftai/fourier_aurora_sdk/bin/fourier_aurora_sdk_examples/get_info
/opt/fftai/fourier_aurora_sdk/bin/fourier_aurora_sdk_examples/set_state
/opt/fftai/fourier_aurora_sdk/bin/fourier_aurora_sdk_examples/set_motion
/opt/fftai/fourier_aurora_sdk/bin/fourier_aurora_sdk_examples/set_arm_swing
/opt/fftai/fourier_aurora_sdk/bin/fourier_aurora_sdk_examples/state_client
/opt/fftai/fourier_aurora_sdk/bin/fourier_aurora_sdk_examples/motion_client
/opt/fftai/fourier_aurora_sdk/bin/fourier_aurora_sdk_examples/control_group_client
```

Python 使用示例位于：

```text
examples/python/usage
```

它们对应最小 C++ 使用示例：

```bash
python3 examples/python/usage/get_info.py 88 test
python3 examples/python/usage/set_state.py 88 test
python3 examples/python/usage/set_motion.py 88 test
python3 examples/python/usage/set_arm_swing.py 88 test
```

## 仓库结构

```text
cmake/                        CMake package 配置模板
examples/                     安装后的 C++ smoke 和 usage 示例
include/                      C++ SDK 公开头文件
src/                          C++ SDK 实现
python/bindings/              pybind11 绑定
python/fourier_aurora_client/ Python 兼容层
tests/                        单元测试
docs/                         SDK 架构与依赖说明
scripts/                      wheel 构建入口
```

## 说明

- `AuroraClient` 是单例。使用不同参数重新创建前需要调用 `close()`。
- Python 保留部分旧版 snake_case 方法，但旧版 `get_motion_state()` 字典接口已移除，请使用 `getMotionState()`。
- `error_codes_pub` 第一阶段先暴露 C++ 接口，Python 辅助接口后续再定。

## 许可证

[Apache 2.0](LICENSE) © Fourier
