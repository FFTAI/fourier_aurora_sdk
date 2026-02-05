# DDS 通信指南

## DDS 简介

OMG 数据分发服务（DDS）是 OMG 提供的中间件协议和 API 标准，用于数据中心连接。它将系统的各个组件集成在一起，提供低延迟数据连接、极高的可靠性以及业务和关键任务物联网（IoT）应用所需的可扩展架构。

在分布式系统中，中间件是位于操作系统和应用程序之间的软件层。它使系统的各个组件能够更轻松地通信和共享数据。它通过让软件开发人员专注于应用程序的特定目的而不是应用程序和系统之间传递信息的机制，简化了分布式系统的开发。

![aurora_dataflow](../../image/DDS_data_centricity.jpg)

(来源: https://www.dds-foundation.org)

## Fourier Aurora Client

**Fourier Aurora Client** 是 Aurora 的 Python 客户端。它允许您通过 DDS 中间件与 Aurora API 交互，从而在 Aurora 平台上检索数据和执行操作。Aurora 客户端可以安装在机器人胸部计算机或任何其他设备上，以与 Aurora 服务器通信。

有关 fourier aurora client 的安装，请参阅快速入门部分（需要链接）。

### 客户端使用

在使用 fourier aurora client 之前，请确保 **Aurora** 已启动。要初始化 fourier aurora client，请使用 `get_instance` 函数。在代码结束时，请使用 `close` 函数进行清理。

```python
from fourier_aurora_client import AuroraClient

client = AuroraClient.get_instance(domain_id=123, robot_name="fouriern1", namespace=None, is_ros_compatible=False)

# 在此处执行内容 ...

client.close()
```
