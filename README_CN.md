# Fourier Aurora SDK

这是**Fourier Aurora**（Advanced Unified Robot Operation and Resource Architecture）的SDK仓库。

如果您是第一次接触Aurora，请阅读以下文档以了解系统的基本信息：

- [Aurora简介](./doc/CN/introduction_CN.md)
- [开发者指南](./doc/CN/developer_guide_CN.md)

## v1.2.0 版本

支持的机器人：

- GR-1P
- GR-2
- GR-3
- Fourier-N1(即将支持)

先决条件：
- 执行器版本：
  - 通信固件版本 0.3.12.31 或更高版本。
  - 驱动固件版本 0.2.10.30 或更高版本。
  - 注意：执行器版本可以使用**FSA助手**进行升级。点击 [FSA助手 for Linux](https://fsa-1302548221.cos.ap-shanghai.myqcloud.com/tool/FSA_Assistant/FSA_Assistant_V0.0.1.24_155_31_x64_Linux_2025-07-08.tar.gz) 下载最新版本。

## 安装

请参考[安装指南](./doc/CN/installation_CN.md)获取安装说明。

## 快速开始

请按照以下步骤使用摇杆提供的控制器开始。如果您不熟悉摇杆，请参考[摇杆教程](./doc/CN/joystick_tutorial_CN.md)了解更多信息。

### 启动容器

您可以在仓库的**根目录**下的终端中运行以下命令来启动容器，如果有必要，请修改`docker_run.bash`中的docker镜像。您可能需要使用`(sudo) docker images`命令来检查系统上可用的docker镜像。

```bash
(sudo) bash docker_run.bash
```
注意：请确保您拥有正确的docker镜像，具体请参阅[安装指南](./doc/CN/installation_CN.md)。

### 运行Aurora前的准备

Aurora可以控制仿真环境中的机器人或真实机器人。在运行Aurora之前，您需要按照以下步骤操作：

#### 1. 仿真运行

对于仿真使用，您需要启动另一个终端并使用以下命令进入刚刚启动的容器：

```bash
(sudo) docker exec -it fourier_aurora_server bash
```

然后，您需要通过运行以下命令启动仿真环境：

```bash
python3 sim/start_simulate_root.py
```
按照指示操作，它将启动仿真环境。机器人将被放置在起始位置。

#### 2. 真实机器人运行

要控制真实机器人，请确保满足以下先决条件：

- 执行器版本兼容。
- 执行器处于开机状态（紫色灯慢闪）。
- 网络通信良好（所有执行器IP可ping通）。
- 机器人处于中性悬挂状态。
- （仅适用于GR-1P在首次创建容器时）使用销钉校准关节。请在所有销钉插入的情况下，在容器中运行`python3 /opt/fftai/fourier_aurora/scripts/calib/gr1t2Calibration.py`。**校准后，请移除销钉。**

### 启动Aurora
要启动Aurora，请在终端中运行以下命令：

```bash
AuroraCore --type 0
```
这里，`--type`指的是Aurora的运行类型，*0*表示在仿真器上运行，*1*表示在真实机器人上运行。

您也可以使用以下命令通过特定的配置文件启动Aurora：

```bash
AuroraCore --config <path_to_your_config_file>
```
此存储库中的`config/config.yaml`中可以找到示例配置文件。

如果程序运行成功，终端中将出现以下消息：

```bash
[info] FSM state run, enter "Default" mode by default
```
此消息表示自检通过（真实机器人）且Aurora程序已准备好运行任务。

### 1. 使用摇杆控制机器人

现在您可以使用摇杆来控制机器人。请参考[摇杆教程](./doc/CN/joystick_tutorial_CN.md)了解摇杆的使用方法。

### 2. 使用客户端控制机器人

您也可以使用Aurora客户端来控制机器人。请参考[客户端教程](./python/README.md)了解客户端的使用方法。

## 问题反馈

请报告任何问题或错误！我们将尽力修复它们。

## 许可证

[Apache 2.0](LICENSE) © Fourier