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
| 诊断客户端 | `examples/client/aurora_client.cpp` | `build/aurora-client` | 系统 `PATH` 中的 `aurora-client` |
| C++ GR3 recipe | `examples/gr3/cpp/` | `build/gr3_{get_info,walk_forward,swing_arms,aurora_state_callback}` | 源码位于 `share/doc/fourier_aurora_sdk/examples/gr3/cpp/` |
| Python GR3 recipe | `examples/gr3/python/` | 从源码运行 | 源码位于 `share/doc/fourier_aurora_sdk/examples/gr3/python/` |

Python wheel 只包含 client package，不包含示例或诊断 executable。

## 运行前检查

1. 启动 Aurora，确认 DDS domain 与 namespace 和 client 一致。
2. 确认安装了兼容的 DDS 与消息 runtime package。
3. 选择符合 `[A-Za-z0-9._-]{1,64}` 的可读 `source_name`。
4. 先运行只读操作，确认机器人型号和当前 FSM。
5. 检查打印的每个 `OperationResult`，不要只看进程退出码。

GR3 recipe 和诊断客户端默认使用 DDS domain `123`。

## 诊断客户端通用约定

统一诊断程序采用 area 和 command 层级：

```text
aurora-client [global options] <area> <command> [command options]
```

DDS domain ID 默认为 `123`，可在命令行任意位置用 `-d`/`--domain_id` 覆盖。
wrapper 会生成进程内默认 Source Name `aurora-client-NNNN`；需要稳定、描述性的
名称时可用 `-s`/`--source_name` 覆盖。Source Name 必须符合
`[A-Za-z0-9._-]{1,64}`。程序启动时会把生效的 domain 和 Source Name 打印到
标准错误。

```bash
aurora-client state get
aurora-client -d 7 -s state-reader state get
aurora-client state get --domain_id 7 --source_name state-reader
```

客户端只启用所选 command 需要的 endpoint。在读取状态、发送 request 或注册租约前，
程序最多等待 5 秒，确认这些 endpoint 已匹配。只读 get 和 monitor 操作不会启用
租约管理、注册租约或占用控制资源。每个操作失败时都会打印完整返回值。

| 退出码 | 含义 |
|---:|---|
| `0` | 所有请求操作成功 |
| `1` | 启动、transport 或操作失败 |
| `2` | 命令行参数错误 |
| `130` | 被 `SIGINT` 或 `SIGTERM` 中断 |

可用 `aurora-client --help`、`aurora-client <area> --help` 或 command 级帮助，
且不会启动 DDS。Debian package 同时安装 Bash、Zsh 和 Fish 自动补全。
Monitor 默认间隔为 `500ms`；`--interval` 可使用无后缀毫秒值或 `ms`/`s`，最小
间隔为 `10ms`。

### `state`

Endpoint 按操作选择：读取和 monitor 使用 `AuroraStateSubscriber`；
`set` 使用 `FsmService`。

```bash
aurora-client state get
aurora-client state list
aurora-client state set 13
aurora-client state set 13 --force
aurora-client state monitor --interval 250ms
```

- `get` 打印机器人信息、switch status 和当前 FSM。
- `list` 打印 Aurora 发布的状态列表。
- `set` 发送数字 FSM request 并打印即时 reply；`--force` 只跳过目标任务的
  `enter_check()`。
- `monitor` 持续读取最新 cache，直到被中断。

### `estimator`

Estimator 是独立的只读 area：

```bash
aurora-client estimator get
aurora-client estimator monitor --interval 1s
```

### `motion`

```bash
aurora-client motion get
aurora-client motion monitor --interval 500
```

下列字段可以组合为一条 `MotionCommand`：

```text
--position <x> <y> <z>
--linear-velocity <vx> <vy> <vz>
--attitude <roll> <pitch> <yaw>
--angular-velocity <wx> <wy> <wz>
--delta-height <value>
--gait-type <uint8>
--skills <s0> <s1> <s2> <s3> <s4> <s5> <s6> <s7>
--duration <seconds>
```

```bash
aurora-client motion set \
  --linear-velocity 0.1 0.0 0.0 --duration 1.0
```

`motion set` 至少需要一个字段，只发布一次，并在 endpoint 匹配后注册
`MotionCommand` 租约。只读调用保持被动，可在其他 source 持有运动租约时监视
Aurora。Motion position 使用世界坐标系，线速度与角速度使用机器人机身坐标系。
数值使用 SI 单位。

### `control-group`

读取和 monitor 操作只使用 `ControlGroupStateSubscriber`。`set` 额外
启用 `ControlGroupCommandPublisher` 和 `ManageLeaseService`。

```bash
aurora-client control-group get
aurora-client control-group monitor
aurora-client control-group set \
  --target left_manipulator 0.0 0.2 0.0 -0.7 \
  --target right_manipulator 0.0 -0.2 0.0 -0.7 \
  --duration 3 --rate 100
```

`get` 打印最新关节和笛卡尔状态/命令。一个或多个重复的 `--target` group 会从
同一实测快照线性插值到目标位置。
`--duration` 默认 5 秒，`--rate` 默认 100 Hz。

发布前，客户端会先等待所选 endpoint 匹配，再注册一个覆盖全部命名关节组的租约。
只读调用不注册租约。Group 名称、向量长度和 mode 是否适用于已连接机器人，由
Aurora 验证。

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
./build/gr3_aurora_state_callback state-callback
./build/gr3_aurora_state_callback state-callback 5  # 5 秒后自动停止
```

可选的正数 duration 适合有界诊断和 CI；不传参数时使用 Ctrl+C 停止。实现位于
`examples/gr3/cpp/aurora_state_callback.cpp`；callback signature 见
[C++ API](cpp_api_CN.md#callback-handler-type)。

### `get_info`

用途：在不获取控制权的情况下验证 GR3 基础连接并检查机器人公布的配置。

这个只读 recipe 选择三个 subscriber，等待最多五秒，然后打印机器人型号、
hardware/end-effector 类型、当前 FSM、机身姿态和速度、stable level 及控制组名称。

```bash
./build/gr3_get_info info-reader
python3 examples/gr3/python/get_info.py info-reader
```

建议先运行它，在不获取控制权的情况下验证连接。

### `walk_forward`

用途：展示完整的有界运动流程，包括租约注册、FSM 切换、运动指令和租约释放。

这个有界 recipe 接收模块 Source Name，注册 `MotionCommand` 资源，请求 GR3 Walk
（FSM `3`），等待状态切换，然后以 `0.3 m/s` 向前运动三秒。获取控制前必须精确
输入 `YES` 确认。

```bash
./build/gr3_walk_forward walk-demo
python3 examples/gr3/python/walk_forward.py walk-demo
```

运行前检查源码中的常量。不要假设 FSM ID 或限制可以直接用于其他机器人型号。

### `swing_arms`

用途：展示从单个测量快照开始插值的、带租约多控制组 Joint 控制。

这个有界 recipe 接收模块 Source Name，租用左右 manipulator 资源，请求 GR3
PdStandStanding（FSM `13`），从同一份快照读取双臂位置，在三秒内插值到中心姿态，
再以 100 Hz 执行两个五秒摆臂周期。运行前必须输入 `YES`。

```bash
./build/gr3_swing_arms arm-demo
python3 examples/gr3/python/swing_arms.py arm-demo
```

示例假设 GR3 的 `left_manipulator` 与 `right_manipulator` group 排列。运行前
必须根据已连接机器人检查并确认关节 index。

### `sub_controller`（Python）

用途：展示手臂 passthrough 指令与 Universal Controller 运行时 rate 和 reset
之间的配合方式。

这个 recipe 接收模块 Source Name，租用左右 manipulator，并请求 GR3
ExternalUniController（FSM `120`）。它先将 momentum 和 waist compensation
rate 设为 `0.0`，再将双臂目标设为全零。用户确认后，将双侧肘关节设为
`-1.0`，momentum rate 设为 `0.5`，waist rate 设为 `1.0`；再次确认后 reset
Universal Controller。每次手臂移动都从最新测量状态开始，以 100 Hz 在两秒内
插值到目标位置。

```bash
python3 examples/gr3/python/sub_controller.py arm-passthrough
```

示例假设 GR3 使用七关节 manipulator 排列，肘关节 index 为 `3`。

## 把示例作为应用模板

每个示例有意把完整流程放在单个文件中。改造成应用时：

1. 用部署配置替换硬编码的机器人能力和 FSM ID。
2. 只选择应用实际需要的 endpoint。
3. 使用可读且能区分并发模块的 `source_name`。
4. 使用受保护 publisher 前注册完整资源集合。
5. 为所有等待和运动设置边界，处理中断，并在所有路径停止 client。
6. 通过状态/错误遥测判断机器人结果；本地 publish 不是 acknowledgement。

完整函数签名见 [C++ API](cpp_api_CN.md) 或 [Python API](python_api_CN.md)。
