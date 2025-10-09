# fourier_aurora_client

这是 **Fourier Aurora** （*高级统一机器人操作与资源架构*）的 Python 客户端。  
它允许你与 Aurora API 交互，以获取数据并在 Aurora 平台上执行操作。

如果你是第一次接触 Aurora，请先阅读以下文档以获得对系统的基本理解：

- [Aurora 简介](../doc/CN/introduction_CN.md)

## 安装

### 依赖项

- Python >= 3.9

对于 [demo_walk.py](./example/gr2/demo_walk.py) 示例，还需要一些额外依赖：

- numpy >= 2.0.0
- torch >= 2.8.0
- pygame == 2.6.1
- ischedule == 1.2.7

### 安装方式

```bash
sudo apt install python3-pip
pip install fourier-aurora-client
```

## 使用方法

Aurora 客户端可以通过 DDS 协议与 Aurora 服务器进行交互。你可以向 Aurora 发送命令，接收来自 Aurora 的状态和传感器数据。在使用客户端之前，请确保 Aurora 服务器正在机器人上运行。你可以参考 [入门指南](../README_CN.md) 了解如何启动 Aurora 服务。

**注意：客户端的 Domain ID 应与服务的 Domain ID 相同。默认的 Domain ID 是 123，可以在启动服务器的配置文件中进行配置。**

### API

关于 fourier aurora client 的 API 规范，请参阅 [API 文档](./docs/CN/API_document_CN.md)。

### 示例

在 [example](./example) 目录下，提供了一些客户端使用示例。

#### demo_client_usage.py

适用于所有机型，演示如何设置 Aurora 的状态和指令。

#### demo_move_joints.py

适用于所有机型，演示如何设置上半身的关节角度。

#### demo_get_states.py

适用于所有机型，演示如何接收 Aurora 发布的状态。

#### demo_walk.py

适用于 GR-2，演示如何在机器人上通过UserCmd任务应用行走策略。

