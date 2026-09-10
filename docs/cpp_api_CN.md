# C++ API 参考

[English](cpp_api.md) · [快速开始](../README_CN.md) ·
[总览](overview_CN.md) · [Python API](python_api_CN.md)

本文说明 Fourier Aurora SDK 面向应用用户的 C++ API。页面先给出紧凑索引，
随后说明应用会使用的公开 Client type 和 function。

```cpp
#include "fourier_aurora_sdk/aurora_client.hpp"
namespace sdk = fourier::aurora_sdk;
```

所有名称位于 `fourier::aurora_sdk`，编译要求 C++17。

## API 索引

### Types

| 类别 | 公共类型 |
|---|---|
| Client 与生命周期 | [`AuroraClient`](#auroraclient)、[`ConnectionOptions`](#connectionoptions)、[`ClientState`](#clientstate)、[`Endpoint`](#endpoint)、[`EndpointStatus`](#endpointstatus)、[`EndpointMatchStatus`](#endpointmatchstatus) |
| 返回值与异常 | [`OperationResult`](#operationresult)、[`AuroraSdkError`](#异常)、[`ConnectionError`](#异常)、[`InvalidArgumentError`](#异常)、[`InvalidStateError`](#异常) |
| 通用与机器人状态 | [`MessageHeader`](#messageheader)、[`RobotInfo`](#robotinfo)、[`StateInfo`](#stateinfo)、[`AuroraState`](#aurorastate) |
| 运动 | [`MotionState`](#motionstate)、[`MotionCommand`](#motioncommand) |
| 控制组 | [`JointControlType`](#jointcontroltype)、[`MotorMode`](#motormode)、[`JointField`](#jointfield)、[`ControlGroupStateEntry`](#controlgroupstateentry)、[`CartesianStateEntry`](#cartesianstateentry)、[`CartesianCommand`](#cartesiancommand)、[`ControlGroupState`](#controlgroupstate)、[`JointCommand`](#jointcommand)、[`ControlGroupCommand`](#controlgroupcommand) |
| 估计器 | [`BaseData`](#basedata)、[`ContactData`](#contactdata)、[`EstimatorData`](#estimatordata) |
| 子控制器 | [`GroupOverrideParam`](#groupoverrideparam)、[`ControllerParam`](#controllerparam)、[`SubControllerCommand`](#subcontrollercommand) |
| 错误 | [`ErrorCodeEntry`](#errorcodeentry)、[`ErrorCodes`](#errorcodes) |
| 租约 | [`AuroraResource`](#auroraresource)、[`LeasePriority`](#leasepriority)、[`LeaseState`](#leasestate)、[`LeaseLossReason`](#leaselossreason)、[`LeaseOption`](#leaseoption)、[`LeaseResult`](#leaseresult)、[`AuthorityResourceStatus`](#authorityresourcestatus) |
| 电机配置 | [`MotorConfigurationMode`](#motorconfigurationmode)、[`MotorConfigurationGroupCommand`](#motorconfigurationgroupcommand)、[`MotorConfigurationCommand`](#motorconfigurationcommand)、[`MotorConfigurationGroupState`](#motorconfigurationgroupstate)、[`MotorConfigurationState`](#motorconfigurationstate) |
| Callback handler | [`AuroraStateHandler`](#callback-handler-type)、[`MotionStateHandler`](#callback-handler-type)、[`ControlGroupStateHandler`](#callback-handler-type)、[`MotorConfigurationStateHandler`](#callback-handler-type)、[`EstimatorDataHandler`](#callback-handler-type)、[`ErrorCodesHandler`](#callback-handler-type)、[`LeaseChangedHandler`](#callback-handler-type) |

### Functions

| 类别 | 公共函数 |
|---|---|
| 生命周期 | [`getInstance`](#getinstance)、[`config`](#config)、[`configure`](#configure)、[`setDomainId`](#setdomainid)、[`domainId`](#domainid)、[`state`](#state)、[`start`](#start)、[`stop`](#stop) |
| Endpoint 就绪 | [`endpointMatchStatus`](#endpointmatchstatus-1)、[`isFsmServiceMatched`](#isfsmservicematched)、[`waitForEndpoints`](#waitforendpoints) |
| 状态读取 | [`getAuroraState`](#getaurorastate)、[`getMotionState`](#getmotionstate)、[`getControlGroupState`](#getcontrolgroupstate)、[`getMotorConfigurationState`](#getmotorconfigurationstate)、[`getEstimatorData`](#getestimatordata)、[`getErrorCodes`](#geterrorcodes)、[`getBaseData`](#getbasedata)、[`getContactData`](#getcontactdata)、[`getStableLevel`](#getstablelevel)、[`getStandPose`](#getstandpose)、[`getJointState`](#getjointstate)、[`getJointCommand`](#getjointcommand)、[`getCartesianState`](#getcartesianstate)、[`getCartesianCommand`](#getcartesiancommand)、[`waitForFsmState`](#waitforfsmstate) |
| 本地指令缓存 | [`getLastMotionCommand`](#getlastmotioncommand)、[`getLastControlGroupCommand`](#getlastcontrolgroupcommand)、[`getLastMotorConfigurationCommand`](#getlastmotorconfigurationcommand)、[`getLastSubControllerCommand`](#getlastsubcontrollercommand) |
| 指令与 Service | [`publishMotionCommand`](#publishmotioncommand)、[`publishControlGroupCommand`](#publishcontrolgroupcommand)、[`publishMotorConfigurationCommand`](#publishmotorconfigurationcommand)、[`setSubController`](#setsubcontroller)、[`requestFsmState`](#requestfsmstate)、[`setVelocity`](#setvelocity)、[`setStandPose`](#setstandpose)、[`resetUnicontroller`](#resetunicontroller) |
| 租约 | [`registerLease`](#registerlease)、[`replaceLease`](#replacelease)、[`releaseLease`](#releaselease)、[`getLease`](#getlease) |
| Callback | [`onAuroraState`](#onaurorastate)、[`onMotionState`](#onmotionstate)、[`onControlGroupState`](#oncontrolgroupstate)、[`onMotorConfigurationState`](#onmotorconfigurationstate)、[`onEstimatorData`](#onestimatordata)、[`onErrorCodes`](#onerrorcodes)、[`onLeaseChanged`](#onleasechanged) |
| 返回值辅助 | [`resultCodeName`、`defaultResultMessage`、`OperationResult::success`、`LeaseResult::success`、`LeaseResult::usable`](#返回值辅助函数) |

## 约定

- `AuroraClient` 是进程级单例，只能通过 `getInstance()` 获取。
- 只能在 `Stopped` 状态修改配置和注册 callback。
- `Running` 只表示本地 DDS 已启动，不代表机器人已经就绪。
- 正常业务失败由 `OperationResult`/`LeaseResult` 返回；异常用于非法调用或致命生命周期错误。
- C++ getter 失败时不修改 output 参数。
- timeout 与租约周期使用 `std::chrono::milliseconds`。
- 机器人数值默认使用 SI 单位。
- Cartesian pose 为 `[x, y, z, qx, qy, qz, qw]`；twist/wrench 为
  `[x, y, z, rx, ry, rz]`。

## Client 与生命周期类型

### `AuroraClient`

可重启的进程级高层 Client，禁止直接构造、复制和移动。所有成员函数见
[Client 函数](#client-函数)。

### `ConnectionOptions`

`start()` 时冻结的完整连接配置。

| 成员 | 类型 | 默认值 | 说明 |
|---|---|---:|---|
| `domain_id` | `uint32_t` | `123` | Client 启动下一个 transport session 时使用的 DDS domain。 |
| `namespace_name` | `std::optional<std::string>` | 未设置 | 显式 DDS namespace；未设置时可由 transport 发现。 |
| `topic_prefix` | `std::string` | `"aurora/"` | 原样添加到 SDK 管理的所有 Aurora Topic 和 Service 名称之前；设为 `""` 表示不加前缀。 |
| `dds_mode` | `uint8_t` | `0` | FourierDDS mode，范围 `0..3`。 |
| `is_ros_compatible` | `std::optional<bool>` | 未设置 | 可选 ROS-compatible naming override。 |
| `use_namespace` | `bool` | `true` | 是否给 endpoint 添加 namespace。 |
| `use_discovery_server` | `std::optional<bool>` | 未设置 | 可选 Discovery Server override。 |
| `service_timeout_ms` | `uint32_t` | `1000` | 同步 Service 默认超时。 |
| `source_name` | `std::string` | `""`（设置前无效） | 必填的可读 client 身份，必须符合 `[A-Za-z0-9._-]{1,64}`。 |
| `allow_emergency_lease_priority` | `bool` | `false` | 允许提出 Emergency 租约请求；最终仍由 Aurora 授权。 |
| `enabled_endpoints` | `std::vector<Endpoint>` | 全部 | `start()` 实际创建的 endpoint。 |

每个 client 都必须配置 `source_name`；`start()` 会在创建 transport 之前校验。
受保护 publisher 还必须启用 `ManageLeaseService`。

### `ClientState`

`Stopped`：无 session、配置可修改；`Starting`：正在启动；`Running`：本地
transport 已运行；`Stopping`：正在 drain 与关闭。

### `Endpoint`

| 枚举值 | 对应 endpoint |
|---|---|
| `FsmService` | FSM 请求 Client。 |
| `ManageLeaseService` | 租约管理 Client。 |
| `MotionCommandPublisher` | 运动指令 Publisher。 |
| `ControlGroupCommandPublisher` | 关节控制组指令 Publisher。 |
| `MotorConfigurationCommandPublisher` | 临时电机配置 Publisher。 |
| `SubControllerService` | Universal Controller 子控制器 Service。 |
| `AuroraStateSubscriber` | Aurora/FSM/权限状态。 |
| `MotionStateSubscriber` | 运动状态。 |
| `ControlGroupStateSubscriber` | 关节与 Cartesian 状态。 |
| `MotorConfigurationStateSubscriber` | 电机配置状态。 |
| `EstimatorDataSubscriber` | Base/contact 估计状态。 |
| `ErrorCodesSubscriber` | 错误码状态。 |

### `EndpointStatus`

`enabled` 表示启动前是否选择该 endpoint；`matched` 表示 DDS 当前是否匹配兼容 peer。

### `EndpointMatchStatus`

每个成员都是对应 endpoint 的 `EndpointStatus`：`fsm_service`、
`manage_lease_service`、`motion_command_publisher`、
`control_group_command_publisher`、`motor_configuration_command_publisher`、
`sub_controller_service`、`aurora_state_subscriber`、
`motion_state_subscriber`、`control_group_state_subscriber`、
`motor_configuration_state_subscriber`、`estimator_data_subscriber`、
`error_codes_subscriber`。

## 返回值与异常类型

### `OperationResult`

| 成员 | 说明 |
|---|---|
| `code` | `uint32_t` 返回码，默认 `CLIENT_ERROR`。 |
| `message` | 人类可读的详细信息。 |
| `success()` | 仅在 `code == SUCCESS` 时为 true。 |

### 异常

| 类型 | 基类 | 说明 |
|---|---|---|
| `AuroraSdkError` | `std::runtime_error` | SDK 异常基类。 |
| `ConnectionError` | `AuroraSdkError` | Transport 启停的致命错误。 |
| `InvalidArgumentError` | `AuroraSdkError` | 参数值、范围或组合非法。 |
| `InvalidStateError` | `AuroraSdkError` | 当前生命周期不允许该操作。 |

每个异常均接受一个 message string。

## 状态与指令类型

### `MessageHeader`

`send_time` 是发送方 `system_clock::time_point`，`frame_id` 是坐标系或来源
label，`received_time` 是 SDK 本地接收时间。

### `RobotInfo`

`robot_type` 表示机器人系列，`hardware_type` 表示硬件/模型版本，
`end_effector_type` 表示末端类型。

### `StateInfo`

`id` 为 FSM ID，`version` 为状态协议/配置版本，`name` 和 `description` 为
用户可读文本，`active` 表示是否为当前状态。

### `AuroraState`

| 成员 | 说明 |
|---|---|
| `header` | 消息时间元数据。 |
| `robot_info` | 已连接机器人身份。 |
| `switch_status` | Aurora 状态切换状态值。 |
| `current_state` | 当前 `StateInfo`。 |
| `available_states` | Aurora 当前报告的状态列表。 |
| `authority_resources` | 当前机器人支持且 SDK 可识别的租约资源及其持有者快照。缺失资源表示不支持；未知的未来资源会被忽略。 |

### `MotionState`

成员包括 `header`；测量/指令成对的 `position`/`position_cmd`、
`lin_velocity`/`lin_velocity_cmd`、`attitude`/`attitude_cmd`、
`ang_velocity`/`ang_velocity_cmd`；高度 `delta_height`/
`delta_height_cmd`；以及 `progress` 和 `stable_level`。所有空间向量长度为 3。

### `MotionCommand`

| 成员 | 说明 |
|---|---|
| `header` | 出站元数据。 |
| `position` | 三维 Base position 请求。 |
| `lin_velocity` | 三维 Base-frame 线速度。 |
| `attitude` | roll/pitch/yaw 请求。 |
| `ang_velocity` | 三维角速度请求。 |
| `delta_height` | 站立高度 offset。 |
| `gait_type` | 机器人特定 gait selector。 |
| `skills` | 8 个机器人特定 skill byte。 |
| `duration` | 指令有效时间，单位秒。 |

### `JointControlType`

`Joint` 表示按逻辑关节索引，`Motor` 表示按硬件电机索引。

### `MotorMode`

`Pd`、`Position`、`Effort` 分别选择 PD、位置和力矩/effort 指令模式。

### `JointField`

`Position`、`Velocity`、`Effort` 用于选择 joint getter 返回的字段。

### `ControlGroupStateEntry`

`group_name` 为控制组名；`joint_position`、`joint_velocity`、
`joint_effort` 是该组的数值 vector。

### `CartesianStateEntry`

`link_name` 为 URDF link；`pose` 是长度 7 的 position + XYZW quaternion；
`twist` 是长度 6 的线/角速度；`wrench` 是长度 6 的力/力矩。

### `CartesianCommand`

`link_name` 是调用方定义的非空查询 key。`pose`、`twist`、`wrench` 默认为空
vector，空表示未指令该字段；至少提供一项，提供时长度分别为 7、6、6。
Aurora 原样保存数值，坐标系由调用方与消费任务管理。

### `ControlGroupState`

`header` 为元数据；`joint_state` 为测量关节；`joint_cmd` 为 Aurora 观测到的
关节指令；`cartesian_state` 为测量 Cartesian 数据；`cartesian_cmd` 为最新观测到
的稀疏 `CartesianCommand` 指令。

### `JointCommand`

`group_name` 指定控制组，`control_type` 指定 joint/motor 索引，`motor_mode`
指定模式；`position`、`velocity`、`effort` 分别为三个指令 vector。

### `ControlGroupCommand`

`header` 为出站元数据，`joint_cmd` 为 `JointCommand` 列表，`cartesian_cmd`
为 `CartesianCommand` 列表。所有目标关节资源都必须被当前租约覆盖；Cartesian
条目还要求 `AuroraResource::CartesianCommand`。

### `BaseData`

| 成员 | 说明 |
|---|---|
| `header` | 消息元数据。 |
| `quat_xyzw`、`quat_wxyz` | 两种顺序的 Base quaternion。 |
| `rpy` | roll、pitch、yaw。 |
| `omega_W`、`acc_W` | World-frame 角速度与线加速度。 |
| `omega_B`、`acc_B` | Base-frame 角速度与线加速度。 |
| `vel_W`、`pos_W` | World-frame 速度与位置。 |
| `vel_B` | Base-frame 速度。 |
| `stable_level` | 估计器稳定等级。 |

Quaternion 长度为 4，其他空间 vector 长度为 3。

### `ContactData`

`header` 为元数据，`contact_force` 和 `contact_prob` 分别是各 contact 的估计
力和概率。

### `EstimatorData`

包含 `BaseData` 与 `ContactData` 的全部 payload 字段：`header`、
`quat_xyzw`、`quat_wxyz`、`rpy`、`omega_W`、`acc_W`、`omega_B`、
`acc_B`、`vel_W`、`pos_W`、`vel_B`、`contact_force`、`contact_prob`、
`stable_level`。

### `GroupOverrideParam`

`group_name` 指定控制组，`control_param` 为控制器特定数值参数。

### `ControllerParam`

`controller_name` 指定 Universal Controller 子控制器，`rate` 为全局激活/
混合比例，`group_override_params` 为每组 override。

### `SubControllerCommand`

`header` 为元数据，`reset` 请求重置，`controller_params` 在非 reset 请求中
携带设置。

### `ErrorCodeEntry`

`high32` 和 `low32` 是一个 Aurora 错误码的高、低 32 bit。

### `ErrorCodes`

`header` 为元数据，`source` 为错误来源，`error_codes` 为当前错误列表。

## 租约类型

### `AuroraResource`

取值：`LeftLeg`、`RightLeg`、`Waist`、`Head`、`LeftManipulator`、
`RightManipulator`、`LeftEndEffector`、`RightEndEffector`、`MotionCommand`、
`CartesianCommand`。

### `LeasePriority`

`Normal` 为默认值；`Emergency` 同时要求 Client opt-in 与 Aurora 授权。

### `LeaseState`

`Unregistered` 无租约；`Registering` 正在注册；`Active` 可用；`Replacing`
正在原子替换资源；`Releasing` 正在 drain/释放；`Lost` 表示旧租约不再可信。

### `LeaseLossReason`

| 取值 | 说明 |
|---|---|
| `None` | 未丢失。 |
| `DeadlineExpired` | steady-clock deadline 到期。 |
| `AuthorityRejected` | Aurora 拒绝租约或 renew。 |
| `OperationTimeout` | 公共租约操作超时。 |
| `InvalidAuthorityResponse` | Authority response 无法使用。 |
| `PublicationDrainTimeout` | 受保护发布未及时 drain。 |
| `InternalError` | 内部错误。 |

### `LeaseOption`

`duration` 默认为 `4000ms`，表示请求周期；`priority` 为可选优先级，未设置
即 Normal；`timeout` 默认为 `500ms`，表示 register/replace 的总 deadline。

### `LeaseResult`

| 成员 | 说明 |
|---|---|
| `operation` | 本次 API 操作的 `OperationResult`。 |
| `state` | 当前本地 `LeaseState`。 |
| `source_name` | 模块身份。 |
| `lease_id` | Authority 分配的 ID。 |
| `resources` | 完整资源集合。 |
| `priority` | 租约优先级。 |
| `granted_duration` | Aurora 授予周期。 |
| `remaining_duration` | 当前剩余时间。 |
| `loss_reason` | Lost 原因。 |
| `success()` | 等同于 `operation.success()`。 |
| `usable()` | 仅当 Active 且剩余时间大于零。 |

### `AuthorityResourceStatus`

`resource` 指定资源，`acquired` 表示是否有 holder，`holder_source_name`
标识 holder，`holder_priority` 为其优先级。该数据仅为
状态快照，实际指令授权使用 SDK 管理的租约。SDK 按资源名称解析并以规范顺序输出条目；允许缺失已知资源，拒绝重复的已知资源，并以限频警告忽略未知的未来资源。

## 电机配置类型

此接口在硬件 SDK 开放前临时由 Aurora SDK 提供，后续将迁移到硬件层 SDK。

### `MotorConfigurationMode`

取值：`Disable`、`Position`、`Velocity`、`Torque`、`Pd`。

### `MotorConfigurationGroupCommand`

`group_name` 指定组，`mode` 指定模式；`position_kp`、`position_ki`、
`position_kd` 为位置环参数；`pd_kp`、`pd_kd` 为 PD 参数。

### `MotorConfigurationCommand`

`header` 为出站元数据，`groups` 为每组配置列表。

### `MotorConfigurationGroupState`

成员为 `group_name`、`mode`、`motor_error`、`position_kp`、
`position_ki`、`position_kd`、`pd_kp`、`pd_kd`。

### `MotorConfigurationState`

`header` 为消息元数据，`groups` 为每组状态列表。

## Callback handler type

下列 alias 定义对应 `AuroraClient::on...` 函数接收的 callback signature。

| Type | 声明 | Payload |
|---|---|---|
| `AuroraStateHandler` | `std::function<void(AuroraState)>` | 最新 Aurora/FSM/权限状态。 |
| `MotionStateHandler` | `std::function<void(MotionState)>` | 最新运动状态。 |
| `ControlGroupStateHandler` | `std::function<void(ControlGroupState)>` | 最新 Joint/Cartesian 状态。 |
| `MotorConfigurationStateHandler` | `std::function<void(MotorConfigurationState)>` | 最新电机配置状态。 |
| `EstimatorDataHandler` | `std::function<void(EstimatorData)>` | 最新 Estimator 数据。 |
| `ErrorCodesHandler` | `std::function<void(ErrorCodes)>` | 最新异步错误码。 |
| `LeaseChangedHandler` | `std::function<void(LeaseResult)>` | 更新后的本地租约快照。 |

传入空 handler 可清空 callback。只能在 Client 为 Stopped 时注册、替换或清空。

## Client 函数

### Client 生命周期

#### `getInstance`

声明：

```cpp
static AuroraClient& getInstance();
```

- 说明：获取进程级单例，不启动 DDS。
- 参数：无。
- 返回：进程级 `AuroraClient` 引用。

#### `config`

声明：

```cpp
ConnectionOptions config() const;
```

- 说明：读取 Client 的标准连接配置。
- 参数：无。
- 返回：`ConnectionOptions` 副本。

#### `configure`

声明：

```cpp
void configure(const ConnectionOptions& options);
```

- 说明：在 Stopped 时替换全部连接配置。
- 参数：`options` — 复制到 Client 的配置。
- 返回：无；配置或生命周期非法时抛出 SDK 异常。

#### `setDomainId`

声明：

```cpp
void setDomainId(uint32_t domain_id);
```

- 说明：`ConnectionOptions::domain_id` 的便捷 setter；更新下个 session 使用的 stopped-stage 标准配置。
- 参数：`domain_id` — DDS domain ID。
- 返回：无；Client 必须为 Stopped。

#### `domainId`

声明：

```cpp
uint32_t domainId() const;
```

- 说明：读取标准配置中的 `ConnectionOptions::domain_id`，是其便捷 getter。
- 参数：无。
- 返回：DDS domain ID。

#### `state`

声明：

```cpp
ClientState state() const;
```

- 说明：读取本地 Client 生命周期状态。
- 参数：无。
- 返回：当前 `ClientState`。

#### `start`

声明：

```cpp
void start();
```

- 说明：校验并冻结配置，然后启动已启用的 DDS endpoint。
- 参数：无。
- 返回：无；致命启动失败时抛出 SDK 异常。

#### `stop`

声明：

```cpp
void stop();
```

- 说明：Drain 当前工作，停止 DDS 并清空 session cache；可重复调用。
- 参数：无。
- 返回：无；致命关闭失败时抛出 SDK 异常。

### Endpoint 就绪

#### `endpointMatchStatus`

声明：

```cpp
EndpointMatchStatus endpointMatchStatus() const;
```

- 说明：读取全部 endpoint 的 enabled/matched 快照。
- 参数：无。
- 返回：`EndpointMatchStatus` 快照。

#### `isFsmServiceMatched`

声明：

```cpp
bool isFsmServiceMatched() const;
```

- 说明：检查已启用的 FSM Service 是否匹配 Server。
- 参数：无。
- 返回：匹配时为 `true`。

#### `waitForEndpoints`

声明：

```cpp
bool waitForEndpoints(std::chrono::milliseconds timeout);
```

- 说明：等待全部已启用 endpoint 匹配后，再等待 200 ms，使 volatile publisher 有时间发送首个 sample；返回成功前会再次检查匹配状态。DDS 发现期间 SDK 每秒打印一次匹配摘要；成功时打印全部匹配信息，超时时打印 error，并列出已匹配和未匹配的 endpoint。
- 参数：`timeout` — DDS 发现的最大等待时间；匹配后的 200 ms 为额外等待。
- 返回：全部匹配为 `true`，超时为 `false`。

### 状态读取

除 wait 外均返回 `OperationResult`，atomic getter 读取一个完整 topic snapshot，
projection getter 只基于同一个 snapshot 派生结果。

#### `getAuroraState`

声明：

```cpp
OperationResult getAuroraState(AuroraState& output) const;
```

- 说明：读取最新完整 Aurora/FSM/权限快照。
- 参数：`output` — 成功时接收快照。
- 返回：操作状态；失败时不修改 `output`。

#### `getMotionState`

声明：

```cpp
OperationResult getMotionState(MotionState& output) const;
```

- 说明：读取最新完整 MotionState 快照。
- 参数：`output` — 成功时接收快照。
- 返回：操作状态；失败时不修改 `output`。

#### `getControlGroupState`

声明：

```cpp
OperationResult getControlGroupState(ControlGroupState& output) const;
```

- 说明：读取最新 Joint/Cartesian 状态快照。
- 参数：`output` — 成功时接收快照。
- 返回：操作状态；失败时不修改 `output`。

#### `getMotorConfigurationState`

声明：

```cpp
OperationResult getMotorConfigurationState(MotorConfigurationState& output) const;
```

- 说明：读取最新临时电机配置状态。
- 参数：`output` — 成功时接收快照。
- 返回：操作状态；失败时不修改 `output`。

#### `getEstimatorData`

声明：

```cpp
OperationResult getEstimatorData(EstimatorData& output) const;
```

- 说明：读取最新完整 Estimator 快照。
- 参数：`output` — 成功时接收快照。
- 返回：操作状态；失败时不修改 `output`。

#### `getErrorCodes`

声明：

```cpp
OperationResult getErrorCodes(ErrorCodes& output) const;
```

- 说明：读取最新异步错误码快照。
- 参数：`output` — 成功时接收快照。
- 返回：操作状态；失败时不修改 `output`。

#### `getBaseData`

声明：

```cpp
OperationResult getBaseData(BaseData& output) const;
```

- 说明：从单个 Estimator 快照投影 Base 数据。
- 参数：`output` — 成功时接收投影结果。
- 返回：操作状态；失败时不修改 `output`。

#### `getContactData`

声明：

```cpp
OperationResult getContactData(ContactData& output) const;
```

- 说明：从单个 Estimator 快照投影 Contact 数据。
- 参数：`output` — 成功时接收投影结果。
- 返回：操作状态；失败时不修改 `output`。

#### `getStableLevel`

声明：

```cpp
OperationResult getStableLevel(uint8_t& output) const;
```

- 说明：从单个 MotionState 快照投影稳定等级。
- 参数：`output` — 成功时接收稳定等级。
- 返回：操作状态；失败时不修改 `output`。

#### `getStandPose`

声明：

```cpp
OperationResult getStandPose(std::vector<double>& output) const;
```

- 说明：从单个 MotionState 投影 `[height, pitch, yaw, stable_level]`。
- 参数：`output` — 成功时接收四元素 vector。
- 返回：操作状态；失败时不修改 `output`。

#### `getJointState`

声明：

```cpp
OperationResult getJointState(const std::string& group_name, JointField field, std::vector<double>& output) const;
```

- 说明：读取指定控制组的一个测量 Joint 字段。
- 参数：`group_name` — 控制组；`field` — position、velocity 或 effort；`output` — 接收 vector。
- 返回：操作状态；失败时不修改 `output`。

#### `getJointCommand`

声明：

```cpp
OperationResult getJointCommand(const std::string& group_name, JointField field, std::vector<double>& output) const;
```

- 说明：读取 Aurora 观测到的指定组 Joint 指令字段。
- 参数：`group_name` — 控制组；`field` — position、velocity 或 effort；`output` — 接收 vector。
- 返回：操作状态；失败时不修改 `output`。

#### `getCartesianState`

声明：

```cpp
OperationResult getCartesianState(const std::string& link_name, CartesianStateEntry& output) const;
```

- 说明：按 URDF link 名读取测量 Cartesian 状态。
- 参数：`link_name` — URDF link；`output` — 接收状态。
- 返回：操作状态；失败时不修改 `output`。

#### `getCartesianCommand`

声明：

```cpp
OperationResult getCartesianCommand(const std::string& link_name, CartesianCommand& output) const;
```

- 说明：按精确 key 读取最新观测到的稀疏 Cartesian 指令。
- 参数：`link_name` — 不透明查询 key；`output` — 接收原始指令。
- 返回：操作状态；失败时不修改 `output`。

#### `waitForFsmState`

声明：

```cpp
bool waitForFsmState(uint32_t state_id, std::chrono::milliseconds timeout);
```

- 说明：等待 Aurora cache 报告指定 FSM ID。
- 参数：`state_id` — 目标 FSM ID；`timeout` — 最大等待时间。
- 返回：观察到目标状态为 `true`，超时为 `false`。

### 本地指令缓存

这些 cache 表示本 Client 的本地意图，不是机器人执行反馈，并会在 `stop()` 时清空。

#### `getLastMotionCommand`

声明：

```cpp
OperationResult getLastMotionCommand(MotionCommand& output) const;
```

- 说明：读取最后发布的 MotionCommand。
- 参数：`output` — 成功时接收 cache。
- 返回：操作状态；失败时不修改 `output`。

#### `getLastControlGroupCommand`

声明：

```cpp
OperationResult getLastControlGroupCommand(ControlGroupCommand& output) const;
```

- 说明：读取最后发布的 ControlGroupCommand。
- 参数：`output` — 成功时接收 cache。
- 返回：操作状态；失败时不修改 `output`。

#### `getLastMotorConfigurationCommand`

声明：

```cpp
OperationResult getLastMotorConfigurationCommand(MotorConfigurationCommand& output) const;
```

- 说明：读取最后发布的 MotorConfigurationCommand。
- 参数：`output` — 成功时接收 cache。
- 返回：操作状态；失败时不修改 `output`。

#### `getLastSubControllerCommand`

声明：

```cpp
OperationResult getLastSubControllerCommand(SubControllerCommand& output) const;
```

- 说明：读取最后成功发送的 SubControllerCommand。
- 参数：`output` — 成功时接收 cache。
- 返回：操作状态；失败时不修改 `output`。

### 指令与 Service

Publish 成功只表示 DDS 接受 sample，不代表机器人已执行。

#### `publishMotionCommand`

声明：

```cpp
OperationResult publishMotionCommand(const MotionCommand& command);
```

- 说明：使用活动 `MotionCommand` 租约发布完整运动指令。
- 参数：`command` — 要发布的运动指令。
- 返回：Publish 或租约检查状态。

#### `publishControlGroupCommand`

声明：

```cpp
OperationResult publishControlGroupCommand(const ControlGroupCommand& command);
```

- 说明：检查全部所需 resource 后发布 Joint 和/或稀疏 Cartesian 指令。
- 参数：`command` — 完整控制组指令。
- 返回：Publish 或租约检查状态。

#### `publishMotorConfigurationCommand`

声明：

```cpp
OperationResult publishMotorConfigurationCommand(const MotorConfigurationCommand& command);
```

- 说明：检查目标 Joint resource 后发布临时电机参数。
- 参数：`command` — 完整电机配置指令。
- 返回：Publish 或租约检查状态。

#### `setSubController`

声明：

```cpp
OperationResult setSubController(const SubControllerCommand& command);
```

- 说明：无租约更新 Universal Controller 子控制器。
- 参数：`command` — reset 或控制器参数请求。
- 返回：Aurora Service 结果；无活动 Universal Controller task 时失败。

#### `requestFsmState`

声明：

```cpp
OperationResult requestFsmState(uint32_t state_id);
OperationResult requestFsmState(uint32_t state_id, bool force_switch);
```

- 说明：无租约请求一个 Aurora FSM 状态。
- 参数：`state_id` — 目标 FSM ID；`force_switch` — 跳过目标任务的
  `enter_check()`，但仍保留所有生命周期检查。
- 返回：Aurora FSM 的详细 admission 结果；目标已激活时返回成功，接受请求后
  状态切换仍异步完成。

#### `setVelocity`

声明：

```cpp
OperationResult setVelocity(double vx, double vy, double yaw, double duration, uint8_t gait_type = 0);
```

- 说明：构造并发布 Base-frame 速度 MotionCommand。
- 参数：`vx`、`vy` — m/s；`yaw` — rad/s；`duration` — 有效秒数；`gait_type` — gait selector。
- 返回：Publish 或租约检查状态。

#### `setStandPose`

声明：

```cpp
OperationResult setStandPose(double delta_height, double delta_pitch, double delta_yaw, double duration = 1.0);
```

- 说明：构造并发布标准站姿 offset。
- 参数：`delta_height` — 米；`delta_pitch`、`delta_yaw` — 弧度；`duration` — 有效秒数。
- 返回：Publish 或租约检查状态。

#### `resetUnicontroller`

声明：

```cpp
OperationResult resetUnicontroller();
```

- 说明：发送 `reset=true` 的子控制器请求。
- 参数：无。
- 返回：Aurora 子控制器 Service 结果。

### 租约管理

活动租约会自动 renew 且不写入磁盘；replace/release 期间暂停受保护发布。

#### `registerLease`

声明：

```cpp
LeaseResult registerLease(const std::vector<AuroraResource>& resources, const LeaseOption& option = {});
```

- 说明：注册模块的第一组完整租约资源。
- 参数：`resources` — 要控制的唯一资源；`option` — 周期、优先级与超时。
- 返回：包含 Aurora 决策和本地租约状态的 `LeaseResult`。

#### `replaceLease`

声明：

```cpp
LeaseResult replaceLease(const std::vector<AuroraResource>& resources, const LeaseOption& option = {});
```

- 说明：原子请求新的完整资源集和 lease ID；明确拒绝时保留仍有效的旧租约。
- 参数：`resources` — 替换资源集；`option` — 周期、优先级与超时。
- 返回：替换操作及最终本地状态的 `LeaseResult`。

#### `releaseLease`

声明：

```cpp
LeaseResult releaseLease();
```

- 说明：释放活动模块租约；重复释放安全。
- 参数：无。
- 返回：释放后的本地 `LeaseResult`。

#### `getLease`

声明：

```cpp
LeaseResult getLease() const;
```

- 说明：不联系 Aurora，读取当前模块租约。
- 参数：无。
- 返回：瞬时本地 `LeaseResult` 快照。

### Callbacks

只能在 Stopped 时注册、替换或清空 callback。状态 callback 在 DDS callback
thread 更新 cache 后 inline 执行；不要在 callback 内调用 `stop()`。

#### `onAuroraState`

声明：

```cpp
void onAuroraState(AuroraStateHandler callback);
```

- 说明：设置或清空 AuroraState cache 更新后的 callback。
- 参数：`callback` — 接收 `AuroraState`；空 handler 表示清空。
- 返回：无；Client 必须为 Stopped。

#### `onMotionState`

声明：

```cpp
void onMotionState(MotionStateHandler callback);
```

- 说明：设置或清空 MotionState callback。
- 参数：`callback` — 接收 `MotionState`；空 handler 表示清空。
- 返回：无；Client 必须为 Stopped。

#### `onControlGroupState`

声明：

```cpp
void onControlGroupState(ControlGroupStateHandler callback);
```

- 说明：设置或清空 ControlGroupState callback。
- 参数：`callback` — 接收 `ControlGroupState`；空 handler 表示清空。
- 返回：无；Client 必须为 Stopped。

#### `onMotorConfigurationState`

声明：

```cpp
void onMotorConfigurationState(MotorConfigurationStateHandler callback);
```

- 说明：设置或清空 MotorConfigurationState callback。
- 参数：`callback` — 接收 `MotorConfigurationState`；空 handler 表示清空。
- 返回：无；Client 必须为 Stopped。

#### `onEstimatorData`

声明：

```cpp
void onEstimatorData(EstimatorDataHandler callback);
```

- 说明：设置或清空 EstimatorData callback。
- 参数：`callback` — 接收 `EstimatorData`；空 handler 表示清空。
- 返回：无；Client 必须为 Stopped。

#### `onErrorCodes`

声明：

```cpp
void onErrorCodes(ErrorCodesHandler callback);
```

- 说明：设置或清空异步 ErrorCodes callback。
- 参数：`callback` — 接收 `ErrorCodes`；空 handler 表示清空。
- 返回：无；Client 必须为 Stopped。

#### `onLeaseChanged`

声明：

```cpp
void onLeaseChanged(LeaseChangedHandler callback);
```

- 说明：设置或清空串行执行的租约变化 callback。
- 参数：`callback` — 接收 `LeaseResult`；空 handler 表示清空。
- 返回：无；Client 必须为 Stopped。

## 返回值辅助函数

`resultCodeName(code)` 返回稳定符号名，未知值返回 `UNKNOWN_RESULT_CODE`；
`defaultResultMessage(code)` 返回默认英文消息；`OperationResult::success()`、
`LeaseResult::success()`、`LeaseResult::usable()` 的语义见对应类型说明。
