# Python API 参考

[English](python_api.md) · [快速开始](../README_CN.md) ·
[总览](overview_CN.md) · [C++ API](cpp_api_CN.md)

Python package 封装与 C++ API 相同的原生 client，并提供 snake_case 函数名：

```python
import fourier_aurora_client as aurora
client = aurora.AuroraClient.get_instance()
```

不要直接构造 `AuroraClient`。Package 要求标准 GIL 的 CPython；当前 Stage 1
wheel 为 CPython 3.10 x86_64 构建。

## 调用约定

- 只能在 `ClientState.STOPPED` 配置单例。
- 始终使用 `try/finally` 并调用 `stop()`。
- 等待函数（`wait_for_endpoints`、`wait_for_fsm_state`）的 timeout 是有限、
  非负的秒数。
- 状态 getter 返回 `(OperationResult, value)`。
- 命令与 service 返回 `OperationResult`，租约函数返回 `LeaseResult`。
- 显式检查每个结果。Publish 成功不表示机器人执行成功。

## 配置与生命周期函数

| 函数 | 返回值 | 用途 |
|---|---|---|
| `AuroraClient.get_instance()` | `AuroraClient` | 返回进程单例，不启动 DDS |
| `config()` | `ConnectionOptions` | 返回当前配置副本 |
| `configure(options)` | `None` | 替换 stopped 状态的完整配置 |
| `set_domain_id(domain_id)` | `None` | 设置下一 session 的 DDS domain，默认 `123` |
| `domain_id()` | `int` | 读取 DDS domain |
| `state()` | `ClientState` | 读取本地生命周期状态 |
| `start()` | `None` | 冻结配置并启动 transport |
| `stop()` | `None` | 等待操作并同步停止 transport |
| `endpoint_match_status()` | `EndpointMatchStatus` | 返回全部 enabled/matched 状态 |
| `is_fsm_service_matched()` | `bool` | 检查启用的 FSM service match |
| `wait_for_endpoints(timeout)` | `bool` | 等待全部已启用 endpoint match |
| `wait_for_fsm_state(state_id, timeout)` | `bool` | 等待缓存 Aurora FSM ID |

`ConnectionOptions` 字段包括 `namespace_name`、`dds_mode`、
`is_ros_compatible`、`use_namespace`、`use_discovery_server`、
`service_timeout_ms`、`source_id`、`source_name`、
`allow_emergency_lease_priority` 和 `enabled_endpoints`。Python 字段与
[C++ 配置表](cpp_api_CN.md#connectionoptions) 的默认值和语义相同。

```python
options = client.config()
options.namespace_name = "robot_1"
options.source_id = 42
options.enabled_endpoints = [
    aurora.Endpoint.MANAGE_LEASE_SERVICE,
    aurora.Endpoint.MOTION_COMMAND_PUBLISHER,
]
client.configure(options)
```

Python endpoint 使用大写 snake case：

```text
FSM_SERVICE                    MANAGE_LEASE_SERVICE
MOTION_COMMAND_PUBLISHER       CONTROL_GROUP_COMMAND_PUBLISHER
MOTOR_CONFIGURATION_COMMAND_PUBLISHER
SUB_CONTROLLER_SERVICE
AURORA_STATE_SUBSCRIBER        MOTION_STATE_SUBSCRIBER
CONTROL_GROUP_STATE_SUBSCRIBER MOTOR_CONFIGURATION_STATE_SUBSCRIBER
ESTIMATOR_DATA_SUBSCRIBER      ERROR_CODES_SUBSCRIBER
```

## 状态 Getter 函数

本节函数都返回 `(result, value)`。Result 失败时应忽略默认构造的 value。

| 函数 | Value | 所需 endpoint |
|---|---|---|
| `get_aurora_state()` | `AuroraState` | `AURORA_STATE_SUBSCRIBER` |
| `get_motion_state()` | `MotionState` | `MOTION_STATE_SUBSCRIBER` |
| `get_control_group_state()` | `ControlGroupState` | `CONTROL_GROUP_STATE_SUBSCRIBER` |
| `get_motor_configuration_state()` | `MotorConfigurationState` | `MOTOR_CONFIGURATION_STATE_SUBSCRIBER` |
| `get_estimator_data()` | `EstimatorData` | `ESTIMATOR_DATA_SUBSCRIBER` |
| `get_error_codes()` | `ErrorCodes` | `ERROR_CODES_SUBSCRIBER` |
| `get_base_data()` | 从 estimator 投影的 `BaseData` | `ESTIMATOR_DATA_SUBSCRIBER` |
| `get_contact_data()` | 从 estimator 投影的 `ContactData` | `ESTIMATOR_DATA_SUBSCRIBER` |
| `get_stable_level()` | Motion state 中的 `int` | `MOTION_STATE_SUBSCRIBER` |
| `get_stand_pose()` | `[height, pitch, yaw, stable_level]` | `MOTION_STATE_SUBSCRIBER` |
| `get_joint_state(group_name, field)` | 选择的实测关节 list | `CONTROL_GROUP_STATE_SUBSCRIBER` |
| `get_joint_command(group_name, field)` | 选择的观测命令 list | `CONTROL_GROUP_STATE_SUBSCRIBER` |
| `get_cartesian_state(link_name)` | `CartesianStateEntry` | `CONTROL_GROUP_STATE_SUBSCRIBER` |
| `get_cartesian_command(link_name)` | `CartesianStateEntry` | `CONTROL_GROUP_STATE_SUBSCRIBER` |

`JointField` 为 `POSITION`、`VELOCITY`、`EFFORT`。笛卡尔状态使用 URDF link
名称。Cartesian command 只是观测状态；当前 SDK 不发布笛卡尔命令。

下列函数返回本 client 的最后一条本地命令 cache：

| 函数 | Value |
|---|---|
| `get_last_motion_command()` | `MotionCommand` |
| `get_last_control_group_command()` | `ControlGroupCommand` |
| `get_last_motor_configuration_command()` | `MotorConfigurationCommand` |
| `get_last_sub_controller_command()` | `SubControllerCommand` |

这些值不是机器人执行反馈，并会在 `stop()` 时清空。

## 命令与 Service 函数

| 函数 | 所需 endpoint/租约 | 用途 |
|---|---|---|
| `publish_motion_command(command)` | Motion publisher；`MOTION_COMMAND` 租约 | 发布完整 `MotionCommand` |
| `publish_control_group_command(command)` | Control-group publisher；所有目标关节资源 | 发布命名关节组命令 |
| `publish_motor_configuration_command(command)` | Motor-config publisher；所有目标关节资源 | 发布临时自定义电机增益 |
| `set_sub_controller(command)` | Sub-controller service；无租约 | 应用/复位 Universal Controller sub-controller 参数 |
| `request_fsm_state(state_id)` | FSM service；无租约 | 请求 Aurora FSM 状态 |
| `set_velocity(vx, vy, yaw, duration, gait_type=0)` | Motion publisher 与租约 | 构造机身坐标系运动命令 |
| `set_stand_pose(height, pitch, yaw, duration=1.0)` | Motion publisher 与租约 | 构造站立偏移命令 |
| `reset_unicontroller()` | Sub-controller service；无租约 | 发送 `SubControllerCommand(reset=True)` |

Active task 不包含 Universal Controller 时，`set_sub_controller()` 返回失败。
Motor configuration 是临时 API，后续会移动到 hardware-level SDK。

### 构造控制组命令

```python
joint = aurora.JointCommand()
joint.group_name = "left_manipulator"
joint.control_type = aurora.JointControlType.JOINT
joint.motor_mode = aurora.MotorMode.PD
joint.position = [0.0, 0.2, 0.0, -0.7]
joint.velocity = [0.0] * len(joint.position)
joint.effort = [0.0] * len(joint.position)

command = aurora.ControlGroupCommand()
command.joint_cmd = [joint]
result = client.publish_control_group_command(command)
```

机器人相关的 group 名称、向量长度、mode、active FSM 和能力由 Aurora 验证。

## 控制租约函数

| 函数 | 返回值 | 用途 |
|---|---|---|
| `register_lease(resources, option=None)` | `LeaseResult` | 注册第一组完整资源 |
| `replace_lease(resources, option=None)` | `LeaseResult` | 原子替换完整资源集合 |
| `release_lease()` | `LeaseResult` | 幂等释放模块租约 |
| `get_lease()` | `LeaseResult` | 读取瞬时本地租约快照 |
| `on_lease_changed(callback)` | `None` | Stopped 时设置租约 callback；传 `None` 清除 |

`AuroraResource` 可选 `LEFT_LEG`、`RIGHT_LEG`、`WAIST`、`HEAD`、
`LEFT_MANIPULATOR`、`RIGHT_MANIPULATOR`、`LEFT_END_EFFECTOR`、
`RIGHT_END_EFFECTOR`、`MOTION_COMMAND`。

`LeaseOption.duration` 与 `.timeout` 使用 `datetime.timedelta`，因为它们映射
C++ chrono duration；默认值分别为两秒和 500 ms。

```python
from datetime import timedelta

option = aurora.LeaseOption()
option.duration = timedelta(seconds=2)
option.timeout = timedelta(milliseconds=500)
option.priority = aurora.LeasePriority.NORMAL

lease = client.register_lease(
    [aurora.AuroraResource.LEFT_MANIPULATOR], option
)
if not lease.usable():
    raise RuntimeError(
        f"lease failed [{lease.operation.code}] {lease.operation.message}"
    )
```

`LeaseResult` 字段为 `operation`、`state`、`source_id`、`source_name`、
`lease_id`、`resources`、`priority`、`granted_duration`、
`remaining_duration`、`loss_reason`。`success()` 描述 API 操作，`usable()` 表示
当前快照现在可以授权命令。SDK 自动续租有效租约，不在磁盘保存租约数据。

本版本 Python 只暴露租约变化 callback。Callback 必须短小、不抛异常，并且不能
调用 `stop()`。

## 导出类型

所有数据类使用“构造后赋值”的方式：

```python
command = aurora.MotionCommand()
command.lin_velocity = [0.1, 0.0, 0.0]
command.duration = 1.0
```

| 领域 | 导出类型 |
|---|---|
| 生命周期 | `ConnectionOptions`、`ClientState`、`Endpoint`、`EndpointStatus`、`EndpointMatchStatus` |
| 返回值 | `OperationResult`、result 常量与辅助函数 |
| 机器人/FSM | `MessageHeader`、`RobotInfo`、`StateInfo`、`AuroraState` |
| 运动 | `MotionCommand`、`MotionState` |
| 控制组 | `JointField`、`JointControlType`、`MotorMode`、`ControlGroupStateEntry`、`CartesianStateEntry`、`JointCommand`、`ControlGroupState`、`ControlGroupCommand` |
| 估计器 | `BaseData`、`ContactData`、`EstimatorData` |
| 电机配置 | `MotorConfigurationMode`、group command/state 与 command/state container 类型 |
| Sub-controller | `GroupOverrideParam`、`ControllerParam`、`SubControllerCommand` |
| 错误 | `ErrorCodeEntry`、`ErrorCodes` |
| 租约 | `AuroraResource`、`LeasePriority`、`LeaseState`、`LeaseLossReason`、`LeaseOption`、`LeaseResult` |

字段含义见 [C++ 数据类型总表](cpp_api_CN.md#数据类型)；Python attribute 使用与
C++ struct 字段相同的 snake_case 名称。原生
`std::chrono::system_clock::time_point` 在 Python 中为 `datetime.datetime`，
duration 为 `datetime.timedelta`。

## 返回值、辅助函数与异常

`OperationResult` 有可写的 `code`、`message` 字段和 `success()` 方法。Package
导出 `SUCCESS` 以及本地 client 常量 `CLIENT_ERROR`、`CLIENT_NOT_RUNNING`、
`ENDPOINT_DISABLED`、`ENDPOINT_UNMATCHED`、`PUBLISH_FAILED`、
`SERVICE_TIMEOUT`、`TRANSPORT_ERROR`、`INVALID_AURORA_RESPONSE`、
`STATE_NOT_RECEIVED`。

使用 `result_code_name(code)` 和 `default_result_message(code)` 查询所有 SDK
已知编号，包括租约和 sub-controller。Aurora service code/message 原样返回。

异常层级与 C++ 对应：

```text
RuntimeError
└── AuroraSdkError
    ├── ConnectionError
    ├── InvalidArgumentError
    └── InvalidStateError
```

普通操作失败在调用点检查返回值；异常只用于非法 API 调用或致命 transport
生命周期失败。
