# Fourier Aurora SDK

这是 **Fourier Aurora** *(高级统一机器人操作与资源架构)* 的SDK仓库。

如果你是第一次接触Aurora，请阅读以下文档了解系统基础知识：

- [Aurora简介](./doc/CN/introduction_CN.md) 
- [开发者指南](./doc/CN/developer_guide_CN.md)

视频介绍请参考：[视频介绍](https://pan.baidu.com/s/1Zcq6ZnGziW1BQBPGOnmG_A?pwd=upiv)

## v1.1.0 版本要求

前提条件：

- 执行器版本：
  - 通信固件版本需 ≥ 0.3.12.31
  - 驱动固件版本需 ≥ 0.2.10.30
  - 注意：执行器版本可通过 **FSA Assistant** 升级。点击 [Linux 版 FSA Assistant](https://fsa-1302548221.cos.ap-shanghai.myqcloud.com/tool/FSA_Assistant/FSA_Assistant_V0.0.1.24_155_31_x64_Linux_2025-07-08.tar.gz) 下载最新版

- 子模块：
  - Aurora 基础环境
  - **fourier_dds** 版本 ≥ 1.1.0
  - **fourier_hardware** 版本 ≥ 1.1.2
  - 注意：Aurora 基础环境已包含在 docker 镜像中，**fourier_dds** 和 **fourier_hardware** 可通过 deb 包安装和升级

## 安装

请参考[安装指南](./doc/CN/installation_CN.md)获取安装说明。

## 快速开始

请按照以下步骤使用摇杆体验提供的控制器功能。如果不熟悉摇杆操作，请参考[摇杆教程](./doc/CN/joystick_tutorial_CN.md)获取更多信息。

### 启动容器

在仓库的**根目录**下运行以下命令启动容器：

```bash
bash docker_run.bash
```

注意：请确保已按照[安装指南](./doc/CN/installation_CN.md)在Docker镜像中安装了fourier-aurora及相关模块。

### 运行Aurora前的准备

Aurora可以控制模拟器或真实机器人。运行Aurora前需要完成以下准备：

#### 模拟器使用

对于模拟器使用，需要另开一个终端并使用以下命令进入已启动的容器：

```bash
(sudo) docker exec -it fourier_aurora_sdk bash
```

然后运行以下命令启动模拟环境：

```bash
python3 sim/start_simulate_root.py
```

按照提示操作，模拟环境将启动，机器人会被放置在起始位置。

#### 真实机器人

控制真实机器人前请确保满足以下条件：

- 执行器版本兼容
- 执行器处于上电状态(紫色灯缓慢闪烁)
- 网络通信正常(所有执行器IP可ping通)
- 机器人处于中立悬挂状态

### 启动 Aurora

在终端运行以下命令启动 Aurora：
```
AuroraCore --name gr2 --type 0
```

其中，`--name` 指 Aurora 的机器人名称，可用名称包括：*gr1*、*gr2*、*gr3*。
`--type` 指运行模式，*0* 为仿真模式，*1* 为实机模式。

如果程序运行成功，终端将显示以下信息：

```bash
[info] FSM state run, enter "Default" mode by default
```

该信息表示自检通过(真实机器人)且Aurora程序已准备好运行任务。

### 使用摇杆控制机器人

现在可以使用摇杆控制机器人。请参考[摇杆教程](./doc/CN/joystick_tutorial_CN.md)了解摇杆使用方法。。

## 问题反馈

如有任何问题或bug，欢迎反馈！我们将尽力修复。

## 许可证

[Apache 2.0](LICENSE) © Fourier
