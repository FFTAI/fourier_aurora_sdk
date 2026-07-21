# 诊断客户端工具

Linux 原生 SDK 包会在以下目录安装三个与机器人型号无关的诊断工具：

```text
/opt/fftai/fourier_aurora_sdk/bin/client/
```

这些 C++ 程序用于调试 SDK 和 Aurora，不包含在 Python wheel 中，也不表示某个
机器人型号的标准操作流程。

## 通用约定

所有工具都要求 DDS domain ID 和 SDK 本地用户标签：

```text
<tool> <domain_id> <user_id> <operation...>
```

`domain_id` 是无符号 32 位 DDS domain；`user_id` 只用于进程内诊断标识，
不是认证信息、路由信息，也不会发送给 Aurora。工具启动时会打印这两个值。

每个工具始终创建下文列出的固定 endpoint 组合。命令行参数只选择操作，不改变
endpoint。诊断工具不会调用聚合的 `waitForEndpoints()`，避免无关 endpoint
阻塞调试；操作失败时会打印完整的 `OperationResult`。

进程退出码：

| 退出码 | 含义 |
|---:|---|
| `0` | 所有操作成功 |
| `1` | 启动、传输或操作失败 |
| `2` | 命令行参数错误 |
| `130` | 收到 `SIGINT` 或 `SIGTERM` |

信号处理函数只设置标志。正常流程退出当前操作后同步调用 `stop()`，再结束进程。

## `state_client`

固定 endpoint：

- `AuroraStateSubscriber`
- `FsmService`

```bash
state_client 123 sdk_debug --get-state
state_client 123 sdk_debug --get-available-states
state_client 123 sdk_debug --request-state 13
state_client 123 sdk_debug --get-state --request-state 13
state_client 123 sdk_debug --monitor-state
```

`--get-state` 打印机器人信息、切换状态和当前 FSM 状态；
`--get-available-states` 打印 Aurora 发布的完整状态列表；
`--request-state` 请求任意数字 FSM ID，并原样打印 Aurora 返回值；
`--monitor-state` 必须单独使用，每 500 ms 读取一次最新缓存快照。

## `motion_client`

固定 endpoint：

- `MotionStateSubscriber`
- `EstimatorDataSubscriber`
- `MotionCommandPublisher`

读取参数：

```text
--get-state
--monitor-state
--get-estimator
--monitor-estimator
```

一次性的 `--get-state` 和 `--get-estimator` 可以组合。monitor 参数必须单独
使用。monitor 每 500 ms 读取最新快照，完整打印数据；临时失败只在返回值变化
时打印，数据恢复后继续输出快照。

`--get-estimator` 和 `--monitor-estimator` 会打印 `EstimatorData` 的所有字段：
两种四元数顺序、RPY、世界/机身坐标系角速度与加速度、世界/机身线速度、世界
位置、接触力/概率、消息头和稳定等级。

下列字段可以组合，最终发布一条 `MotionCommand`：

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

所有数值默认使用 SI 单位。运动位置使用世界坐标系，线速度和角速度使用机器人
机身坐标系。本地发布成功不等于 Aurora 已执行命令。

## `control_group_client`

固定 endpoint：

- `ControlGroupStateSubscriber`
- `ControlGroupCommandPublisher`

```bash
control_group_client 123 sdk_debug --get-state
control_group_client 123 sdk_debug --monitor-state
control_group_client 123 sdk_debug \
  --position left_manipulator 0.0 0.2 0.0 -0.7 \
  --position right_manipulator 0.0 -0.2 0.0 -0.7 \
  --duration 3 --rate 100
```

`--get-state` 打印最新关节与笛卡尔状态/命令；`--monitor-state` 必须单独使用，
每 500 ms 读取最新快照。一个或多个 `--position` 会从同一份
`ControlGroupState` 快照线性插值到目标关节位置。`--duration` 默认 5 秒，
`--rate` 默认 100 Hz。

SDK 会转发可表示的 group 名称、向量长度、控制模式和数值。具体机器人上的
语义合法性仍由 Aurora 校验。
