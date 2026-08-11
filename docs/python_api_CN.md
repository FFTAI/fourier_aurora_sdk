# Python API 参考

[English](python_api.md) · [快速开始](../README_CN.md) ·
[总览](overview_CN.md) · [C++ API](cpp_api_CN.md)

本文说明 `fourier_aurora_client` 面向应用用户的 API。页面首先列出导出的
type 和 function，随后逐项说明 Client method 与可写 attribute。

```python
import fourier_aurora_client as aurora
client = aurora.AuroraClient.get_instance()
```

不要直接构造 `AuroraClient`。当前 stage-one wheel 面向 standard-GIL
CPython 3.10 x86_64。

## API 索引

### Types

| 类别 | 导出类型 |
|---|---|
| Client 与生命周期 | [`AuroraClient`](#auroraclient)、[`ConnectionOptions`](#connectionoptions)、[`ClientState`](#clientstate)、[`Endpoint`](#endpoint)、[`EndpointStatus`](#endpointstatus)、[`EndpointMatchStatus`](#endpointmatchstatus) |
| 返回值与异常 | [`OperationResult`](#operationresult)、[`AuroraSdkError`](#异常)、[`ConnectionError`](#异常)、[`InvalidArgumentError`](#异常)、[`InvalidStateError`](#异常) |
| 通用与机器人状态 | [`MessageHeader`](#messageheader)、[`RobotInfo`](#robotinfo)、[`StateInfo`](#stateinfo)、[`AuroraState`](#aurorastate) |
| 运动 | [`MotionState`](#motionstate)、[`MotionCommand`](#motioncommand) |
| 控制组 | [`JointControlType`](#jointcontroltype)、[`MotorMode`](#motormode)、[`JointField`](#jointfield)、[`ControlGroupStateEntry`](#controlgroupstateentry)、[`CartesianStateEntry`](#cartesianstateentry)、[`ControlGroupState`](#controlgroupstate)、[`JointCommand`](#jointcommand)、[`ControlGroupCommand`](#controlgroupcommand) |
| 估计器 | [`BaseData`](#basedata)、[`ContactData`](#contactdata)、[`EstimatorData`](#estimatordata) |
| 子控制器 | [`GroupOverrideParam`](#groupoverrideparam)、[`ControllerParam`](#controllerparam)、[`SubControllerCommand`](#subcontrollercommand) |
| 错误 | [`ErrorCodeEntry`](#errorcodeentry)、[`ErrorCodes`](#errorcodes) |
| 租约 | [`AuroraResource`](#auroraresource)、[`LeasePriority`](#leasepriority)、[`LeaseState`](#leasestate)、[`LeaseLossReason`](#leaselossreason)、[`LeaseOption`](#leaseoption)、[`LeaseResult`](#leaseresult) |
| 电机配置 | [`MotorConfigurationMode`](#motorconfigurationmode)、[`MotorConfigurationGroupCommand`](#motorconfigurationgroupcommand)、[`MotorConfigurationCommand`](#motorconfigurationcommand)、[`MotorConfigurationGroupState`](#motorconfigurationgroupstate)、[`MotorConfigurationState`](#motorconfigurationstate) |

### Functions

| 类别 | 公共函数 |
|---|---|
| 生命周期 | [`get_instance`](#get_instance)、[`config`](#config)、[`configure`](#configure)、[`set_domain_id`](#set_domain_id)、[`domain_id`](#domain_id)、[`state`](#state)、[`start`](#start)、[`stop`](#stop) |
| Endpoint 就绪 | [`endpoint_match_status`](#endpoint_match_status)、[`is_fsm_service_matched`](#is_fsm_service_matched)、[`wait_for_endpoints`](#wait_for_endpoints) |
| 状态读取 | [`get_aurora_state`](#get_aurora_state)、[`get_motion_state`](#get_motion_state)、[`get_control_group_state`](#get_control_group_state)、[`get_motor_configuration_state`](#get_motor_configuration_state)、[`get_estimator_data`](#get_estimator_data)、[`get_error_codes`](#get_error_codes)、[`get_base_data`](#get_base_data)、[`get_contact_data`](#get_contact_data)、[`get_stable_level`](#get_stable_level)、[`get_stand_pose`](#get_stand_pose)、[`get_joint_state`](#get_joint_state)、[`get_joint_command`](#get_joint_command)、[`get_cartesian_state`](#get_cartesian_state)、[`get_cartesian_command`](#get_cartesian_command)、[`wait_for_fsm_state`](#wait_for_fsm_state) |
| 本地指令缓存 | [`get_last_motion_command`](#get_last_motion_command)、[`get_last_control_group_command`](#get_last_control_group_command)、[`get_last_motor_configuration_command`](#get_last_motor_configuration_command)、[`get_last_sub_controller_command`](#get_last_sub_controller_command) |
| 指令与 Service | [`publish_motion_command`](#publish_motion_command)、[`publish_control_group_command`](#publish_control_group_command)、[`publish_motor_configuration_command`](#publish_motor_configuration_command)、[`set_sub_controller`](#set_sub_controller)、[`request_fsm_state`](#request_fsm_state)、[`set_velocity`](#set_velocity)、[`set_stand_pose`](#set_stand_pose)、[`reset_unicontroller`](#reset_unicontroller) |
| 租约 | [`register_lease`](#register_lease)、[`replace_lease`](#replace_lease)、[`release_lease`](#release_lease)、[`get_lease`](#get_lease)、[`on_lease_changed`](#on_lease_changed) |
| 返回值辅助 | [`result_code_name`、`default_result_message`、`OperationResult.success`、`LeaseResult.success`、`LeaseResult.usable`](#返回值辅助函数) |

## 约定

- 只能在 `ClientState.STOPPED` 时配置进程级单例。
- 使用 `try/finally` 并显式调用 `stop()`。
- wait timeout 是有限、非负的秒数。
- 状态与 cache getter 返回 `(OperationResult, value)`。
- 指令/Service 返回 `OperationResult`，租约函数返回 `LeaseResult`。
- 正常业务失败不是异常，调用方必须检查返回值。
- C++ time point 映射为 `datetime.datetime`，duration 映射为
  `datetime.timedelta`。
- 数值默认使用 SI 单位；Cartesian pose 为
  `[x, y, z, qx, qy, qz, qw]`。

## Client 与生命周期类型

### `AuroraClient`

Native 单例的进程级 Python wrapper。使用 `get_instance()`；直接构造会抛出
`TypeError`。全部 method 见 [Client 函数](#client-函数)。

### `ConnectionOptions`

使用 `aurora.ConnectionOptions()` 构造，传入 `configure()` 时复制并校验。

| Attribute | Python 类型 | 默认值 | 说明 |
|---|---|---:|---|
| `domain_id` | `int` | `123` | Client 启动下一个 transport session 时使用的 DDS domain，必须符合 `uint32`。 |
| `namespace_name` | `str | None` | `None` | 显式 DDS namespace。 |
| `topic_prefix` | `str` | `"aurora/"` | 原样添加到 SDK 管理的所有 Aurora Topic 和 Service 名称之前；设为 `""` 表示不加前缀。 |
| `dds_mode` | `int` | `0` | FourierDDS mode，仅允许 `0..3`。 |
| `is_ros_compatible` | `bool | None` | `None` | 可选 ROS-compatible naming override。 |
| `use_namespace` | `bool` | `True` | 是否使用 namespace。 |
| `use_discovery_server` | `bool | None` | `None` | 可选 Discovery Server override。 |
| `service_timeout_ms` | `int` | `1000` | 同步 Service 默认超时。 |
| `source_id` | `int` | `0` | 部署分配的模块 ID，必须符合 `uint32`。 |
| `source_name` | `str | None` | `None` | 诊断名称，默认 `aurora_sdk_<source_id>`。 |
| `allow_emergency_lease_priority` | `bool` | `False` | 允许 Emergency 请求；仍需 Aurora 授权。 |
| `enabled_endpoints` | `list[Endpoint]` | 全部 | `start()` 创建的 endpoint。 |

启用租约或受保护 publisher 时 `source_id` 必须非零；受保护 publisher 还必须
启用 `MANAGE_LEASE_SERVICE`。

### `ClientState`

`STOPPED`：无 session；`STARTING`：正在启动；`RUNNING`：本地 transport
运行；`STOPPING`：正在 drain/关闭。

### `Endpoint`

| 枚举值 | Endpoint |
|---|---|
| `FSM_SERVICE` | FSM 请求 Service。 |
| `MANAGE_LEASE_SERVICE` | 租约管理 Service。 |
| `MOTION_COMMAND_PUBLISHER` | 运动指令 Publisher。 |
| `CONTROL_GROUP_COMMAND_PUBLISHER` | 关节控制组 Publisher。 |
| `MOTOR_CONFIGURATION_COMMAND_PUBLISHER` | 临时电机配置 Publisher。 |
| `SUB_CONTROLLER_SERVICE` | Universal Controller 子控制器 Service。 |
| `AURORA_STATE_SUBSCRIBER` | Aurora/FSM/权限状态。 |
| `MOTION_STATE_SUBSCRIBER` | 运动状态。 |
| `CONTROL_GROUP_STATE_SUBSCRIBER` | Joint/Cartesian 状态。 |
| `MOTOR_CONFIGURATION_STATE_SUBSCRIBER` | 电机配置状态。 |
| `ESTIMATOR_DATA_SUBSCRIBER` | Base/contact 估计状态。 |
| `ERROR_CODES_SUBSCRIBER` | 错误码状态。 |

### `EndpointStatus`

`enabled` 表示启动前是否选择该 endpoint；`matched` 表示 DDS 是否匹配 peer。

### `EndpointMatchStatus`

全部 attribute 均为 `EndpointStatus`：`fsm_service`、
`manage_lease_service`、`motion_command_publisher`、
`control_group_command_publisher`、`motor_configuration_command_publisher`、
`sub_controller_service`、`aurora_state_subscriber`、
`motion_state_subscriber`、`control_group_state_subscriber`、
`motor_configuration_state_subscriber`、`estimator_data_subscriber`、
`error_codes_subscriber`。

## 返回值与异常类型

### `OperationResult`

`code` 是可写整数返回码，默认 `CLIENT_ERROR`；`message` 是可写详情；
`success()` 仅在 `code == SUCCESS` 时为 true。

### 异常

`AuroraSdkError` 继承 `RuntimeError`；`ConnectionError` 表示致命 transport
启停失败；`InvalidArgumentError` 表示类型、数值、范围或组合非法；
`InvalidStateError` 表示生命周期调用非法。后三者均继承 `AuroraSdkError`。

## 状态与指令类型

下列数据类型都可无参构造，attribute 均可写。

### `MessageHeader`

`send_time` 和 `received_time` 是 `datetime.datetime`，分别表示发送时间与
SDK 本地接收时间；`frame_id` 是坐标系或来源字符串。

### `RobotInfo`

`robot_type` 表示机器人系列，`hardware_type` 表示硬件/模型版本，
`end_effector_type` 表示末端类型。

### `StateInfo`

`id` 为 FSM ID，`version` 为协议/配置版本，`name` 和 `description` 为用户可读
文本，`active` 表示是否当前激活。

### `AuroraState`

| Attribute | 说明 |
|---|---|
| `header` | `MessageHeader`。 |
| `robot_info` | `RobotInfo`。 |
| `switch_status` | Aurora 状态切换值。 |
| `current_state` | 当前 `StateInfo`。 |
| `available_states` | 可用 `StateInfo` 列表。 |
| `authority_resources` | 每项包含 `resource`、`acquired`、`holder_source_id`、`holder_source_name`、`holder_priority` 的权限快照。 |

### `MotionState`

包含 `header`；测量/指令成对的 `position`/`position_cmd`、
`lin_velocity`/`lin_velocity_cmd`、`attitude`/`attitude_cmd`、
`ang_velocity`/`ang_velocity_cmd`；`delta_height`/`delta_height_cmd`；以及
`progress`、`stable_level`。空间向量长度均为 3。

### `MotionCommand`

`header` 为元数据；`position`、`lin_velocity`、`attitude`、
`ang_velocity` 均为三元素数组；`delta_height` 为高度 offset；`gait_type`
为 gait selector；`skills` 为 8 byte；`duration` 为有效时间（秒，默认 1.0）。

### `JointControlType`

`JOINT` 按逻辑关节索引；`MOTOR` 按硬件电机索引。

### `MotorMode`

取值：`PD`、`POSITION`、`EFFORT`。

### `JointField`

取值：`POSITION`、`VELOCITY`、`EFFORT`。

### `ControlGroupStateEntry`

`group_name` 为控制组名；`joint_position`、`joint_velocity`、
`joint_effort` 为对应数值列表。

### `CartesianStateEntry`

`link_name` 为 URDF link；`pose` 为七元素 position + XYZW quaternion；
`twist` 为六元素线/角速度；`wrench` 为六元素力/力矩。

### `ControlGroupState`

`header` 为元数据；`joint_state` 为测量关节列表；`joint_cmd` 为观测到的指令；
`cartesian_state` 为测量 Cartesian 列表；`cartesian_cmd` 为观测到的
Cartesian 指令，本版本只读。

### `JointCommand`

`group_name` 指定控制组，`control_type` 指定 joint/motor 索引，
`motor_mode` 指定模式，`position`、`velocity`、`effort` 为三个指令列表。

### `ControlGroupCommand`

`header` 为出站元数据；`joint_cmd` 为 `JointCommand` 列表。

### `BaseData`

| Attribute | 说明 |
|---|---|
| `header` | 消息元数据。 |
| `quat_xyzw`、`quat_wxyz` | 两种顺序的四元素 Base quaternion。 |
| `rpy` | roll、pitch、yaw。 |
| `omega_W`、`acc_W` | World-frame 角速度和线加速度。 |
| `omega_B`、`acc_B` | Base-frame 角速度和线加速度。 |
| `vel_W`、`pos_W` | World-frame 速度和位置。 |
| `vel_B` | Base-frame 速度。 |
| `stable_level` | 稳定等级。 |

### `ContactData`

`header` 为元数据；`contact_force`、`contact_prob` 为各 contact 的力与概率列表。

### `EstimatorData`

全部 attribute：`header`、`quat_xyzw`、`quat_wxyz`、`rpy`、`omega_W`、
`acc_W`、`omega_B`、`acc_B`、`vel_W`、`pos_W`、`vel_B`、
`contact_force`、`contact_prob`、`stable_level`，含义与上述 Base/contact 相同。

### `GroupOverrideParam`

`group_name` 指定控制组，`control_param` 为控制器特定参数列表。

### `ControllerParam`

`controller_name` 指定子控制器，`rate` 为全局激活/混合比例，
`group_override_params` 为每组 override 列表。

### `SubControllerCommand`

`header` 为元数据，`reset` 请求重置，`controller_params` 在非 reset 时携带设置。

### `ErrorCodeEntry`

`high32` 与 `low32` 是一个错误码的高、低 32 bit。

### `ErrorCodes`

`header` 为元数据，`source` 为错误来源，`error_codes` 为当前错误列表。

## 租约类型

### `AuroraResource`

取值：`LEFT_LEG`、`RIGHT_LEG`、`WAIST`、`HEAD`、`LEFT_MANIPULATOR`、
`RIGHT_MANIPULATOR`、`LEFT_END_EFFECTOR`、`RIGHT_END_EFFECTOR`、
`MOTION_COMMAND`。

### `LeasePriority`

取值：`NORMAL`、`EMERGENCY`。Emergency 同时要求 Client opt-in 和 Aurora 授权。

### `LeaseState`

`UNREGISTERED` 无租约；`REGISTERING` 正在注册；`ACTIVE` 可用；
`REPLACING` 正在原子替换；`RELEASING` 正在释放；`LOST` 表示旧租约不可信。

### `LeaseLossReason`

取值：`NONE`、`DEADLINE_EXPIRED`、`AUTHORITY_REJECTED`、
`OPERATION_TIMEOUT`、`INVALID_AUTHORITY_RESPONSE`、
`PUBLICATION_DRAIN_TIMEOUT`、`INTERNAL_ERROR`。

### `LeaseOption`

`duration` 是 `datetime.timedelta`，默认 4 秒；`priority` 为可选
`LeasePriority`，默认 `None`（Normal）；`timeout` 为 `datetime.timedelta`，
默认 500 ms。

### `LeaseResult`

| Attribute/method | 说明 |
|---|---|
| `operation` | 本次调用的 `OperationResult`。 |
| `state` | 当前 `LeaseState`。 |
| `source_id`、`source_name` | 模块身份。 |
| `lease_id` | Authority 分配 ID。 |
| `resources` | 完整 `AuroraResource` 列表。 |
| `priority` | 租约优先级。 |
| `granted_duration` | 授予周期。 |
| `remaining_duration` | 当前剩余时间。 |
| `loss_reason` | `LeaseLossReason`。 |
| `success()` | 检查内嵌 operation。 |
| `usable()` | 仅 Active 且剩余时间大于零。 |

## 电机配置类型

此接口后续将迁移到硬件层 SDK。

### `MotorConfigurationMode`

取值：`DISABLE`、`POSITION`、`VELOCITY`、`TORQUE`、`PD`。

### `MotorConfigurationGroupCommand`

Attribute：`group_name`、`mode`、`position_kp`、`position_ki`、
`position_kd`、`pd_kp`、`pd_kd`；gain 列表长度应匹配目标组。

### `MotorConfigurationCommand`

`header` 为出站元数据，`groups` 为 group command 列表。

### `MotorConfigurationGroupState`

Attribute：`group_name`、`mode`、`motor_error`、`position_kp`、
`position_ki`、`position_kd`、`pd_kp`、`pd_kd`。

### `MotorConfigurationState`

`header` 为元数据，`groups` 为 group state 列表。

## Client 函数

### Client 生命周期

#### `get_instance`

声明：

```python
AuroraClient.get_instance()
```

- 说明：获取进程级单例，不启动 DDS。
- 参数：无。
- 返回：进程级 `AuroraClient`。

#### `config`

声明：

```python
client.config()
```

- 说明：读取标准连接配置。
- 参数：无。
- 返回：`ConnectionOptions` 副本。

#### `configure`

声明：

```python
client.configure(options)
```

- 说明：在 Stopped 时替换全部连接配置。
- 参数：`options` — 复制到 Client 的 `ConnectionOptions`。
- 返回：`None`；配置或生命周期非法时抛出 SDK 异常。

#### `set_domain_id`

声明：

```python
client.set_domain_id(domain_id)
```

- 说明：`ConnectionOptions.domain_id` 的便捷 setter；更新下个 session 使用的 stopped-stage 标准配置。
- 参数：`domain_id` — `uint32` 范围内的整数。
- 返回：`None`；Client 必须为 Stopped。

#### `domain_id`

声明：

```python
client.domain_id()
```

- 说明：读取标准配置中的 `ConnectionOptions.domain_id`，是其便捷 getter。
- 参数：无。
- 返回：domain integer。

#### `state`

声明：

```python
client.state()
```

- 说明：读取本地 Client 生命周期状态。
- 参数：无。
- 返回：当前 `ClientState`。

#### `start`

声明：

```python
client.start()
```

- 说明：校验并冻结配置，然后启动已启用 endpoint。
- 参数：无。
- 返回：`None`；致命启动失败时抛出 SDK 异常。

#### `stop`

声明：

```python
client.stop()
```

- 说明：Drain 工作，停止 DDS 并清空 session cache；可重复调用。
- 参数：无。
- 返回：`None`；致命关闭失败时抛出 SDK 异常。

### Endpoint 就绪

#### `endpoint_match_status`

声明：

```python
client.endpoint_match_status()
```

- 说明：读取全部 endpoint 的 enabled/matched 快照。
- 参数：无。
- 返回：`EndpointMatchStatus`。

#### `is_fsm_service_matched`

声明：

```python
client.is_fsm_service_matched()
```

- 说明：检查已启用 FSM Service 是否匹配 Server。
- 参数：无。
- 返回：匹配时为 `True`。

#### `wait_for_endpoints`

声明：

```python
client.wait_for_endpoints(timeout)
```

- 说明：等待全部已启用 endpoint 匹配。
- 参数：`timeout` — 有限、非负的秒数。
- 返回：全部匹配为 `True`，超时为 `False`。

### 状态读取

除 wait 外均返回 `(OperationResult, value)`；失败时忽略 value。

#### `get_aurora_state`

声明：

```python
client.get_aurora_state()
```

- 说明：读取最新完整 Aurora/FSM/权限快照。
- 参数：无。
- 返回：`(OperationResult, AuroraState)`。

#### `get_motion_state`

声明：

```python
client.get_motion_state()
```

- 说明：读取最新完整 MotionState 快照。
- 参数：无。
- 返回：`(OperationResult, MotionState)`。

#### `get_control_group_state`

声明：

```python
client.get_control_group_state()
```

- 说明：读取最新完整 Joint/Cartesian 快照。
- 参数：无。
- 返回：`(OperationResult, ControlGroupState)`。

#### `get_motor_configuration_state`

声明：

```python
client.get_motor_configuration_state()
```

- 说明：读取最新临时电机配置状态。
- 参数：无。
- 返回：`(OperationResult, MotorConfigurationState)`。

#### `get_estimator_data`

声明：

```python
client.get_estimator_data()
```

- 说明：读取最新完整 Estimator 快照。
- 参数：无。
- 返回：`(OperationResult, EstimatorData)`。

#### `get_error_codes`

声明：

```python
client.get_error_codes()
```

- 说明：读取最新异步错误码快照。
- 参数：无。
- 返回：`(OperationResult, ErrorCodes)`。

#### `get_base_data`

声明：

```python
client.get_base_data()
```

- 说明：从一个 Estimator 快照投影 Base 数据。
- 参数：无。
- 返回：`(OperationResult, BaseData)`。

#### `get_contact_data`

声明：

```python
client.get_contact_data()
```

- 说明：从一个 Estimator 快照投影 Contact 数据。
- 参数：无。
- 返回：`(OperationResult, ContactData)`。

#### `get_stable_level`

声明：

```python
client.get_stable_level()
```

- 说明：从一个 MotionState 快照投影稳定等级。
- 参数：无。
- 返回：`(OperationResult, int)`。

#### `get_stand_pose`

声明：

```python
client.get_stand_pose()
```

- 说明：从一个 MotionState 投影 `[height, pitch, yaw, stable_level]`。
- 参数：无。
- 返回：包含四个值的 `(OperationResult, list[float])`。

#### `get_joint_state`

声明：

```python
client.get_joint_state(group_name, field)
```

- 说明：读取指定控制组的一个测量 Joint 字段。
- 参数：`group_name` — 控制组；`field` — `JointField` selector。
- 返回：`(OperationResult, list[float])`。

#### `get_joint_command`

声明：

```python
client.get_joint_command(group_name, field)
```

- 说明：读取 Aurora 观测到的指定组 Joint 指令字段。
- 参数：`group_name` — 控制组；`field` — `JointField` selector。
- 返回：`(OperationResult, list[float])`。

#### `get_cartesian_state`

声明：

```python
client.get_cartesian_state(link_name)
```

- 说明：按 URDF link 名读取测量 Cartesian 状态。
- 参数：`link_name` — 目标 URDF link。
- 返回：`(OperationResult, CartesianStateEntry)`。

#### `get_cartesian_command`

声明：

```python
client.get_cartesian_command(link_name)
```

- 说明：按 URDF link 名读取观测到的 Cartesian 指令。
- 参数：`link_name` — 目标 URDF link。
- 返回：`(OperationResult, CartesianStateEntry)`。

#### `wait_for_fsm_state`

声明：

```python
client.wait_for_fsm_state(state_id, timeout)
```

- 说明：等待 Aurora cache 报告指定 FSM ID。
- 参数：`state_id` — `uint32` FSM ID；`timeout` — 有限、非负秒数。
- 返回：观察到目标状态为 `True`，超时为 `False`。

### 本地指令缓存

这些 cache 表示本地 intent，不是机器人执行反馈，并在 `stop()` 时清空。

#### `get_last_motion_command`

声明：

```python
client.get_last_motion_command()
```

- 说明：读取最后发布的 MotionCommand。
- 参数：无。
- 返回：`(OperationResult, MotionCommand)`。

#### `get_last_control_group_command`

声明：

```python
client.get_last_control_group_command()
```

- 说明：读取最后发布的 ControlGroupCommand。
- 参数：无。
- 返回：`(OperationResult, ControlGroupCommand)`。

#### `get_last_motor_configuration_command`

声明：

```python
client.get_last_motor_configuration_command()
```

- 说明：读取最后发布的 MotorConfigurationCommand。
- 参数：无。
- 返回：`(OperationResult, MotorConfigurationCommand)`。

#### `get_last_sub_controller_command`

声明：

```python
client.get_last_sub_controller_command()
```

- 说明：读取最后成功发送的 SubControllerCommand。
- 参数：无。
- 返回：`(OperationResult, SubControllerCommand)`。

### 指令与 Service

Publish 成功只表示 DDS 接受 sample，不代表机器人已执行。

#### `publish_motion_command`

声明：

```python
client.publish_motion_command(command)
```

- 说明：使用活动 Motion lease 发布完整运动指令。
- 参数：`command` — 要发布的 `MotionCommand`。
- 返回：Publish 与租约检查的 `OperationResult`。

#### `publish_control_group_command`

声明：

```python
client.publish_control_group_command(command)
```

- 说明：检查全部目标 resource 后发布 Joint 指令。
- 参数：`command` — 要发布的 `ControlGroupCommand`。
- 返回：Publish 与租约检查的 `OperationResult`。

#### `publish_motor_configuration_command`

声明：

```python
client.publish_motor_configuration_command(command)
```

- 说明：检查目标 resource 后发布临时电机参数。
- 参数：`command` — 要发布的 `MotorConfigurationCommand`。
- 返回：Publish 与租约检查的 `OperationResult`。

#### `set_sub_controller`

声明：

```python
client.set_sub_controller(command)
```

- 说明：无租约更新 Universal Controller 子控制器。
- 参数：`command` — reset 或参数 `SubControllerCommand`。
- 返回：Aurora 的 `OperationResult`；无活动 Universal Controller task 时失败。

#### `request_fsm_state`

声明：

```python
client.request_fsm_state(state_id)
```

- 说明：无租约请求一个 Aurora FSM 状态。
- 参数：`state_id` — `uint32` 范围内整数。
- 返回：Aurora FSM Service 的 `OperationResult`。

#### `set_velocity`

声明：

```python
client.set_velocity(vx, vy, yaw, duration, gait_type=0)
```

- 说明：构造并发布 Base-frame 速度 MotionCommand。
- 参数：`vx`、`vy` — m/s；`yaw` — rad/s；`duration` — 有效秒数；`gait_type` — gait selector。
- 返回：Publish 与租约检查的 `OperationResult`。

#### `set_stand_pose`

声明：

```python
client.set_stand_pose(delta_height, delta_pitch, delta_yaw, duration=1.0)
```

- 说明：构造并发布标准站姿 offset。
- 参数：`delta_height` — 米；`delta_pitch`、`delta_yaw` — 弧度；`duration` — 有效秒数。
- 返回：Publish 与租约检查的 `OperationResult`。

#### `reset_unicontroller`

声明：

```python
client.reset_unicontroller()
```

- 说明：发送 `reset=True` 的子控制器请求。
- 参数：无。
- 返回：Aurora 子控制器 `OperationResult`。

### 租约管理

租约自动 renew 且不落盘；lease callback 应保持简短，不要调用 `stop()`。

#### `register_lease`

声明：

```python
client.register_lease(resources, option=None)
```

- 说明：注册模块的第一组完整租约资源。
- 参数：`resources` — 唯一 `AuroraResource` 列表；`option` — 可选 `LeaseOption`。
- 返回：包含 Aurora 决策与本地状态的 `LeaseResult`。

#### `replace_lease`

声明：

```python
client.replace_lease(resources, option=None)
```

- 说明：原子请求新完整资源集和 lease ID；明确拒绝时保留仍有效的旧租约。
- 参数：`resources` — 替换资源；`option` — 可选 `LeaseOption`。
- 返回：替换操作与最终本地状态的 `LeaseResult`。

#### `release_lease`

声明：

```python
client.release_lease()
```

- 说明：释放活动模块租约；重复释放安全。
- 参数：无。
- 返回：释放后的 `LeaseResult`。

#### `get_lease`

声明：

```python
client.get_lease()
```

- 说明：不联系 Aurora，读取当前模块租约。
- 参数：无。
- 返回：瞬时本地 `LeaseResult` 快照。

#### `on_lease_changed`

声明：

```python
client.on_lease_changed(callback)
```

- 说明：在 Stopped 时设置串行执行的租约变化 callback。
- 参数：`callback` — 接收 `LeaseResult` 的 callable；传 `None` 清空。
- 返回：`None`。

## 返回值辅助函数

`result_code_name(code)` 返回稳定符号名；`default_result_message(code)` 返回默认
英文消息；`OperationResult.success()`、`LeaseResult.success()`、
`LeaseResult.usable()` 的语义见上述类型。
