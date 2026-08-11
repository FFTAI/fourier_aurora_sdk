# 示例与诊断客户端

[English](examples_and_clients.md) · [快速开始](../README_CN.md) ·
[总览](overview_CN.md) · [C++ API](cpp_api_CN.md) ·
[Python API](python_api_CN.md)

仓库提供两类可运行程序：

- 用于检查 endpoint 和执行单项测试的、与机器人型号无关的 C++ 诊断客户端；
- 展示 C++/Python 应用结构的简短 GR3 recipe。

这些程序是学习和诊断工具，不是自主安全控制器。运行运动示例前，应支撑机器人、
清空工作区域、确认机器人型号与 FSM ID，并随时准备停止机器人。

## 文件位置

| 程序 | 源码 | 原生构建结果 | Debian 安装结果 |
|---|---|---|---|
| 诊断客户端 | `examples/client/` | `build/{state_client,motion_client,control_group_client}` | `/opt/fftai/fourier_aurora_sdk/bin/client/` |
| C++ GR3 recipe | `examples/gr3/cpp/` | `build/gr3_{get_info,walk_forward,swing_arms,aurora_state_callback}` | 源码位于 `share/doc/fourier_aurora_sdk/examples/gr3/cpp/` |
| Python GR3 recipe | `examples/gr3/python/` | 从源码运行 | 源码位于 `share/doc/fourier_aurora_sdk/examples/gr3/python/` |

Python wheel 只包含 client package，不包含示例或诊断 executable。

## 运行前检查

1. 启动 Aurora，确认 DDS domain 与 namespace 和 client 一致。
2. 确认安装了兼容的 DDS 与消息 runtime package。
3. 发送受保护命令前，取得部署分配的非零 `source_id`。
4. 先运行只读操作，确认机器人型号和当前 FSM。
5. 检查打印的每个 `OperationResult`，不要只看进程退出码。

GR3 recipe 使用 DDS domain `123`；诊断客户端从命令行接收 domain。

## 诊断客户端通用约定

每个工具都要求 DDS domain ID 和非零模块 `source_id`：

```text
<tool> <domain_id> <source_id> <operation...>
```

两者都是无符号 32 位整数。`source_id` 是租约 authority 使用的模块身份，同时
连接的模块必须使用不同 ID。工具启动时会打印 domain 与 source。

每个工具创建固定 endpoint 组合；选项只选择操作，不改变 DDS endpoint。工具
不会调用聚合的 `waitForEndpoints()`，避免无关 endpoint 阻塞单项诊断。每个操作
失败时都会打印完整返回值。

| 退出码 | 含义 |
|---:|---|
| `0` | 所有请求操作成功 |
| `1` | 启动、transport 或操作失败 |
| `2` | 命令行参数错误 |
| `130` | 被 `SIGINT` 或 `SIGTERM` 中断 |

### `state_client`

Endpoint：`AuroraStateSubscriber`、`FsmService`。

```bash
state_client 123 42 --get-state
state_client 123 42 --get-available-states
state_client 123 42 --request-state 13
state_client 123 42 --get-state --request-state 13
state_client 123 42 --monitor-state
```

- `--get-state` 打印机器人信息、switch status 和当前 FSM。
- `--get-available-states` 打印 Aurora 发布的状态列表。
- `--request-state` 发送数字 FSM 请求并打印 Aurora reply。
- `--monitor-state` 必须单独使用，每 500 ms 读取一次最新 cache。

### `motion_client`

Endpoint：`MotionStateSubscriber`、`EstimatorDataSubscriber`、
`MotionCommandPublisher`、`ManageLeaseService`。

读取操作：

```bash
motion_client 123 42 --get-state
motion_client 123 42 --get-estimator
motion_client 123 42 --get-state --get-estimator
motion_client 123 42 --monitor-state
motion_client 123 42 --monitor-estimator
```

Monitor 选项必须单独使用。Monitor 每 500 ms 读取最新 cache，只在临时失败发生
变化时重复打印。

下列字段可以组合为一条 `MotionCommand`：

```text
--position <x> <y> <z>
--lin-velocity <vx> <vy> <vz>
--attitude <roll> <pitch> <yaw>
--ang-velocity <wx> <wy> <wz>
--delta-height <value>
--gait-type <uint8>
--skills <s0> <s1> <s2> <s3> <s4> <s5> <s6> <s7>
--duration <seconds>
```

```bash
motion_client 123 42 \
  --get-state --get-estimator \
  --lin-velocity 0.1 0.0 0.0 --duration 1.0
```

客户端发布前注册 `MotionCommand` 租约。Motion position 使用世界坐标系，线速度
与角速度使用机器人机身坐标系。数值使用 SI 单位。

### `control_group_client`

Endpoint：`ControlGroupStateSubscriber`、`ControlGroupCommandPublisher`、
`ManageLeaseService`。

```bash
control_group_client 123 42 --get-state
control_group_client 123 42 --monitor-state
control_group_client 123 42 \
  --position left_manipulator 0.0 0.2 0.0 -0.7 \
  --position right_manipulator 0.0 -0.2 0.0 -0.7 \
  --duration 3 --rate 100
```

`--get-state` 打印最新关节和笛卡尔状态/命令；`--monitor-state` 必须单独使用。
一个或多个 `--position` group 会从同一实测快照线性插值到目标位置。
`--duration` 默认 5 秒，`--rate` 默认 100 Hz。

发布前，客户端注册一个覆盖全部命名关节组的租约。Group 名称、向量长度和 mode
是否适用于已连接机器人，由 Aurora 验证。

## GR3 Recipe

使用 `FOURIER_AURORA_SDK_BUILD_EXAMPLES=ON`（原生默认值）构建 C++ recipe。
Python recipe 要求已安装 `fourier_aurora_client` wheel，或者在 `PYTHONPATH`
中提供等价开发 package。

### `aurora_state_callback`

用途：展示不获取控制权的只读 callback 订阅。示例只创建
`AuroraStateSubscriber`，统计每个状态 sample，并在每 10 次 callback 时打印
消息发送时间、本地 SDK 接收时间和累计数量。两个时间都使用
`HH:MM:SS:ffffff` 格式。

```bash
cmake --build build --target gr3_aurora_state_callback
./build/gr3_aurora_state_callback
./build/gr3_aurora_state_callback 5  # 5 秒后自动停止
```

可选的正数 duration 适合有界诊断和 CI；不传参数时使用 Ctrl+C 停止。实现位于
`examples/gr3/cpp/aurora_state_callback.cpp`；callback signature 见
[C++ API](cpp_api_CN.md#callback-handler-type)。

### `get_info`

用途：在不获取控制权的情况下验证 GR3 基础连接并检查机器人公布的配置。

这个只读 recipe 选择三个 subscriber，等待最多五秒，然后打印机器人型号、
hardware/end-effector 类型、当前 FSM、机身姿态和速度、stable level 及控制组名称。

```bash
./build/gr3_get_info
python3 examples/gr3/python/get_info.py
```

建议先运行它，在不获取控制权的情况下验证连接。

### `walk_forward`

用途：展示完整的有界运动流程，包括租约注册、FSM 切换、运动指令和租约释放。

这个有界 recipe 接收模块 source ID，注册 `MotionCommand` 资源，请求 GR3 Walk
（FSM `3`），等待状态切换，然后以 `0.3 m/s` 向前运动三秒。获取控制前必须精确
输入 `YES` 确认。

```bash
./build/gr3_walk_forward 42
python3 examples/gr3/python/walk_forward.py 42
```

运行前检查源码中的常量。不要假设 FSM ID 或限制可以直接用于其他机器人型号。

### `swing_arms`

用途：展示从单个测量快照开始插值的、带租约多控制组 Joint 控制。

这个有界 recipe 接收模块 source ID，租用左右 manipulator 资源，请求 GR3
PdStandStanding（FSM `13`），从同一份快照读取双臂位置，在三秒内插值到中心姿态，
再以 100 Hz 执行两个五秒摆臂周期。运行前必须输入 `YES`。

```bash
./build/gr3_swing_arms 42
python3 examples/gr3/python/swing_arms.py 42
```

示例假设 GR3 的 `left_manipulator` 与 `right_manipulator` group 排列。运行前
必须根据已连接机器人检查并确认关节 index。

### `sub_controller`（Python）

用途：展示手臂 passthrough 指令与 Universal Controller 运行时 rate 和 reset
之间的配合方式。

这个 recipe 接收模块 source ID，租用左右 manipulator，并请求 GR3
ExternalUniController（FSM `120`）。它先将 momentum 和 waist compensation
rate 设为 `0.0`，再将双臂目标设为全零。用户确认后，将双侧肘关节设为
`-1.0`，momentum rate 设为 `0.5`，waist rate 设为 `1.0`；再次确认后 reset
Universal Controller。每次手臂移动都从最新测量状态开始，以 100 Hz 在两秒内
插值到目标位置。

```bash
python3 examples/gr3/python/sub_controller.py 42
```

示例假设 GR3 使用七关节 manipulator 排列，肘关节 index 为 `3`。

## 把示例作为应用模板

每个示例有意把完整流程放在单个文件中。改造成应用时：

1. 用部署配置替换硬编码的机器人能力和 FSM ID。
2. 只选择应用实际需要的 endpoint。
3. 使用登记过的模块 `source_id`，不要在运行时随机生成。
4. 使用受保护 publisher 前注册完整资源集合。
5. 为所有等待和运动设置边界，处理中断，并在所有路径停止 client。
6. 通过状态/错误遥测判断机器人结果；本地 publish 不是 acknowledgement。

完整函数签名见 [C++ API](cpp_api_CN.md) 或 [Python API](python_api_CN.md)。
