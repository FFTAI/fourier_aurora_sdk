# fourier_aurora_client

这是 **Fourier Aurora** （*高级统一机器人操作与资源架构*）的 Python 客户端。  
它允许你与 Aurora API 交互，以获取数据并在 Aurora 平台上执行操作。

如果你是第一次接触 Aurora，请先阅读以下文档以获得对系统的基本理解：

- [Aurora 简介](../doc/CN/introduction_CN.md)

## 安装

### 依赖项

- Python >= 3.9

对于 [demo_walk.py](../example/gr2/demo_walk.py) 示例，还需要一些额外依赖：

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

### API

关于 fourier aurora client 的 API 规范，请参阅 [API 文档](./docs/CN/API_document_CN.md)。

### 示例

在 [example](../example) 目录下，提供了一些客户端使用示例。

#### demo_client_usage.py

适用于所有机型，演示如何设置 Aurora 的状态和指令。

#### demo_move_joints.py

适用于所有机型，演示如何设置上半身的关节角度。

#### demo_get_states.py

适用于所有机型，演示如何接收 Aurora 发布的状态。

#### demo_walk.py

适用于 GR-2，演示如何在机器人上通过UserCmd任务应用行走策略。

### 注意事项

如果 Aurora 运行在 docker 容器中，而客户端在宿主机上运行，请设置以下环境变量：

```bash
export FASTDDS_BUILTIN_TRANSPORTS=UDPv4
```