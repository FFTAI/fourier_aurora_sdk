# GR3-Mini 主板开发用例

[English](README.md)

适用 GR3-Mini `v3_0_0`、SDK `1.0.1` 及匹配的机器人环境。优先在机器人主板的配套运行环境中开发；只需本 SDK 仓库，不需要 Aurora、AMC 源码或模型训练环境。若交付使用容器，以下检查与运行都在同一个配套容器中完成。

## 先检查已有环境

```bash
uname -m
python3 --version
python3 -m pip show fourier-aurora-client
python3 -c 'from fourier_aurora_client import AuroraClient; print("SDK import OK")'
dpkg-query -W fourier_dds fourier-dds-msgs fourier-aurora-sdk
aurora-client --help
```

Python 使用标准 GIL CPython 3.10–3.13；原生依赖版本为 `fourier_dds 1.2.0-1`、`fourier-dds-msgs 0.2.0-1`。CPU 架构、Python ABI 和 wheel 必须匹配。wheel 不包含 DDS 原生库和 CLI，CLI 随 Debian SDK 安装。已有安装正确时无需重装。使用脚本的同一 Python 解释器检查，避免在另一个虚拟环境安装后仍用系统 Python 启动。

缺少组件时，先从机器人配套安装介质或交付支持获取匹配的依赖/安装包并校验摘要，按[依赖与安装](../../docs/overview_CN.md#依赖)修复。仅克隆 SDK 仓库不能补齐机器人运行时或 DDS 依赖。不要替换配套 FastDDS/消息库来消除版本错误。虚拟环境会隔离系统 Python 包，创建后需要安装匹配 SDK wheel；系统原生依赖仍必需。手柄脚本还需要交付环境提供的 PyYAML、rich、pygame 及设备访问权限，这些不是 SDK wheel 的依赖。

## Python 用例

从 SDK 仓库根目录执行。示例默认只读，不申请租约、不切换状态：

```bash
python3 examples/gr3mini/python/gr3mini_client.py --help
python3 examples/gr3mini/python/gr3mini_client.py read --domain 123
```

domain、namespace 必须与机器人部署一致；有 namespace 时添加 `--namespace NAME`。身份由示例内部配置，通常无需传 `--source`；只有同时运行多个实例时才用不同名称。

确认机器人按交付操作流程准备完成、型号匹配且已处于目标状态后，才运行运动用例。先退出手柄，避免租约冲突：

```bash
# 已处于 110302 或 110311；需要人工输入 YES 确认
python3 examples/gr3mini/python/gr3mini_client.py walk --domain 123
# 已处于 201；小幅头部动作，需要人工输入 YES 确认
python3 examples/gr3mini/python/gr3mini_client.py head --domain 123
```

程序检查状态、反馈新鲜度与租约，结束时尝试停止运动并释放租约。它不负责状态准备，不实现急停，也不提供起身控制。阅读 [Python 源码](python/gr3mini_client.py) 后再运行运动命令。

## C++ 只读用例

需要 C++17 编译器、CMake >= 3.16 及安装好的 SDK 开发文件。按部署调整 [cpp/read_state.cpp](cpp/read_state.cpp) 中的 domain 和连接配置，然后构建：

```bash
cmake -S examples/gr3mini -B build-gr3mini \
  -DCMAKE_PREFIX_PATH='/opt/fftai/fourier_aurora_sdk;/opt/fftai'
cmake --build build-gr3mini --parallel 2
./build-gr3mini/gr3mini_read_state
```

自定义安装前缀应替换上述路径。找不到 CMake package 时检查 SDK DEB 和前缀；导入或运行出现共享库错误时检查原生运行库安装和系统动态链接器配置，详见[构建说明](../../docs/overview_CN.md)。构建 SDK 本身的步骤也在该说明中，不要求重新编译机器人控制程序。

Python 提供读取、行走、头部控制三个流程，C++ 提供只读生命周期对照。文档中心下载副本应与这里的对应源码保持一致；接口完整定义见 [Python API](../../docs/python_api_CN.md) 和 [C++ API](../../docs/cpp_api_CN.md)。这些用例未经本次实机验证。
