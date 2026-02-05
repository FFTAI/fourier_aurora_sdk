# 开发者指南

本指南面向希望在傅里叶机器人上使用 Fourier Aurora SDK 开发自己的应用程序的开发者。它根据您项目的需求，提供了如何使用 SDK 的信息。

Aurora 运行一个 FSM 状态机，每个状态对应特定的任务（控制器）。您可以使用手柄或通过 DDS 接口发送命令在这些状态之间切换。每个任务都有自己的输入和输出。您可以使用**手柄**或 **DDS** 与任务交互。

## 状态和任务概览

Aurora 原生提供各种控制器，每个控制器由一个任务运行。控制器不能直接调用。相反，它们被封装在单独的状态中，这确保了控制器不会重叠。

状态名称          | 手柄映射 | DDS 映射 | 链接
--------------------|------------------|-------------|-------------
Default             | LB+RB            | 0           | [初始状态](controller_reference/default_state.md)
Security Protection | LT+RT            | 9           | [安全保护状态](controller_reference/security_protection_state.md)
Joint Stand         | LB+A             | 1           | [关节直立状态](controller_reference/joint_stand_state.md)
PdStand             | LB+A             | 2           | [Pd站立状态](controller_reference/pd_stand_state.md)
RL Locomotion       | RB+A             | 3           | [RL行走状态](controller_reference/rl_locomotion_state.md)
UserCmd             | 无               | 10          | [用户指令状态](controller_reference/user_cmd_state.md)
Upper UserCmd       | 无               | 11          | [上身用户指令状态](controller_reference/upper_user_cmd_state.md)

每个状态的状态明细格式相同。以下是状态明细中术语的解释。

- **State Name（状态名称）**: 状态的名称。
- **Task Name（任务名称）**: 任务的名称，通常指控制器。
- **Joystick mapping（手柄映射）**: 使用手柄切换到该状态的组合键。
- **DDS mapping（DDS 映射）**: 用于通过 DDS 消息或客户端切换到该状态的 DDS 映射值。
- **Frequency（频率）**: 该状态下控制器的默认频率。
- **Avaliable for hanging（可用于悬挂）**: 某些控制器（主要是基于强化学习的控制器）在脚离地时可能表现危险。本节指示该状态下的控制器是否适合悬挂。
- **Avaliable for standing（可用于站立）**: 本节指示该状态下的控制器是否能够独立站立。
- **Auto Protection Switch（自动保护切换）**: 某些控制器带有自动保护切换机制。当它们发现自己超出可控范围时，它们将自动切换到安全保护状态。本节指示该状态下的控制器是否带有自动保护切换机制。

## Fourier Aurora Client

**Fourier Aurora Client** 是 Aurora 的 Python 客户端。它允许您通过 DDS 中间件与 Aurora API 交互，从而在 Aurora 平台上检索数据和执行操作。Aurora 客户端可以安装在机器人胸部计算机或任何其他设备上，以与 Aurora 服务器通信。

有关 fourier aurora client 的安装，请参阅快速入门部分（需要链接）。

### 客户端使用

在使用 fourier aurora client 之前，请确保 **Aurora** 已启动。要初始化 fourier aurora client，请使用 `get_instance` 函数。*domain_id* 和 *robot_name* 参数是与 Aurora 连接的必需参数。对于 fouriern1，*domain_id* 默认设置为 123，*robot_name* 应为 "fouriern1"。*namespace* 和 *is_ros_compatible* 选项保留供将来使用。

在代码结束时，请使用 `close` 函数对客户端进行清理。

```python
from fourier_aurora_client import AuroraClient

client = AuroraClient.get_instance(domain_id=123, robot_name="fouriern1", namespace=None, is_ros_compatible=False)

# 在此处执行内容 ...

client.close()
```

### 状态切换接口

Aurora 中的每个控制器状态由一个 *DDS 映射值*映射，该值可以在上表中找到。开发者可以使用 fourier aurora client 中的 `set_fsm_state` 函数在这些状态之间切换并获取当前运行状态。

```python
client.set_fsm_state(2)     # 切换到 pdstand 状态

state = client.get_fsm_state()   # 获取当前状态的 DDS 映射值
```

对于 Rl locomotion 状态，其上身控制器由上身状态管理器管理。可以通过 `set_upper_fsm_state` 函数进行切换。

```python
client.set_upper_fsm_state(1)     # 切换到 upper act 状态（摆臂）

state = client.get_upper_fsm_state()   # 获取当前上身状态的 DDS 映射值
```

Aurora 使用速度源值来确保只有一个设备控制机器人的速度。在发送速度命令之前，使用 `set_velocity_source` 切换速度源。

```python
client.set_velocity_source(2)   # 切换到客户端控制

client.get_velocity_source()    # 获取当前速度源
```


