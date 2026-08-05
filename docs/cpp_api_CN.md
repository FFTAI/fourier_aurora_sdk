# C++ API 参考

[English](cpp_api.md) · [快速开始](../README_CN.md) ·
[总览](overview_CN.md) · [Python API](python_api_CN.md)

受支持的高层 C++ API 由以下头文件声明：

```cpp
#include "fourier_aurora_sdk/aurora_client.hpp"
namespace sdk = fourier::aurora_sdk;
```

所有公开类型都位于 `fourier::aurora_sdk`，动态库要求 C++17。

## 调用约定

- `AuroraClient` 是进程级单例，使用 `getInstance()` 获取。
- 只能在 `Stopped` 状态配置 client 和 callback。
- `start()`/`stop()` 管理本地 transport；必须显式调用 `stop()`。
- Getter 与命令返回 `OperationResult`；正常操作失败不使用异常。
- Getter 失败时不会修改输出参数。
- Timeout 使用 `std::chrono::milliseconds`，可以传入
  `std::chrono::seconds(5)` 等兼容 duration。
- 除字段另有说明外，机器人数值使用 SI 单位。

## Client 配置与生命周期

| 函数 | 返回值 | 用途 |
|---|---|---|
| `AuroraClient::getInstance()` | `AuroraClient&` | 返回单例，不启动 DDS |
| `config()` | `ConnectionOptions` | 返回当前配置的副本 |
| `configure(options)` | `void` | 在 stopped 状态替换完整配置 |
| `setDomainId(id)` | `void` | 设置下一次 session 使用的 DDS domain，默认 `123` |
| `domainId()` | `uint32_t` | 读取 DDS domain |
| `state()` | `ClientState` | 读取本地生命周期状态 |
| `start()` | `void` | 冻结配置并启动 transport |
| `stop()` | `void` | 等待已进入操作并同步停止 transport |
| `endpointMatchStatus()` | `EndpointMatchStatus` | 返回所有 endpoint 的 enabled/matched 状态 |
| `isFsmServiceMatched()` | `bool` | 直接检查已启用 FSM service 的匹配状态 |
| `waitForEndpoints(timeout)` | `bool` | 等待所有已启用 endpoint 匹配 |

`ClientState` 包含 `Stopped`、`Starting`、`Running`、`Stopping`。
`Running` 是本地状态，不表示机器人 ready。

### `ConnectionOptions`

| 字段 | 默认值 | 含义 |
|---|---|---|
| `namespace_name` | 未设置 | 可选 DDS namespace 覆盖 |
| `dds_mode` | `0` | FourierDDS mode，有效值 `0..3` |
| `is_ros_compatible` | 未设置 | 可选 ROS compatibility 覆盖 |
| `use_namespace` | `true` | 使用带 namespace 的 endpoint |
| `use_discovery_server` | 未设置 | 可选 discovery server 覆盖 |
| `service_timeout_ms` | `1000` | 同步 service 默认 timeout |
| `source_id` | `0` | 部署分配的租约模块身份 |
| `source_name` | 未设置 | 诊断名称，默认 `aurora_sdk_<source_id>` |
| `allow_emergency_lease_priority` | `false` | 显式允许该 client 申请 emergency 优先级 |
| `enabled_endpoints` | 全部 endpoint | `start()` 精确创建的 endpoint |

启用租约管理或受保护 publisher 时，`source_id` 必须非零。受保护 publisher 还
必须同时启用 `ManageLeaseService`。

`Endpoint` 可选值：

```text
FsmService                    ManageLeaseService
MotionCommandPublisher        ControlGroupCommandPublisher
MotorConfigurationCommandPublisher
SubControllerService
AuroraStateSubscriber         MotionStateSubscriber
ControlGroupStateSubscriber   MotorConfigurationStateSubscriber
EstimatorDataSubscriber       ErrorCodesSubscriber
```

## 状态 Getter

原子 getter 返回一个 DDS 状态 topic 的最新完整缓存：

| 函数 | 输出 | 所需 endpoint |
|---|---|---|
| `getAuroraState(output)` | `AuroraState` | `AuroraStateSubscriber` |
| `getMotionState(output)` | `MotionState` | `MotionStateSubscriber` |
| `getControlGroupState(output)` | `ControlGroupState` | `ControlGroupStateSubscriber` |
| `getMotorConfigurationState(output)` | `MotorConfigurationState` | `MotorConfigurationStateSubscriber` |
| `getEstimatorData(output)` | `EstimatorData` | `EstimatorDataSubscriber` |
| `getErrorCodes(output)` | `ErrorCodes` | `ErrorCodesSubscriber` |

下列 getter 从一份 topic 快照中投影所需内容：

| 函数 | 输出/行为 | 来源快照 |
|---|---|---|
| `getBaseData(output)` | 机身位置、速度、加速度、姿态、stable level | `EstimatorData` |
| `getContactData(output)` | 接触力与接触概率 | `EstimatorData` |
| `getStableLevel(output)` | Stable level | `MotionState` |
| `getStandPose(output)` | `[delta_height, pitch, yaw, stable_level]` | `MotionState` |
| `getJointState(group, field, output)` | 实测位置、速度或力矩 | `ControlGroupState.joint_state` |
| `getJointCommand(group, field, output)` | 观测到的目标位置、速度或力矩 | `ControlGroupState.joint_cmd` |
| `getCartesianState(link, output)` | URDF link 的实测 pose、twist、wrench | `ControlGroupState.cartesian_state` |
| `getCartesianCommand(link, output)` | 观测到的 link 笛卡尔命令 | `ControlGroupState.cartesian_cmd` |
| `waitForFsmState(id, timeout)` | 等待缓存 FSM ID 等于 `id` | `AuroraState` |

`JointField` 为 `Position`、`Velocity` 或 `Effort`。当前版本笛卡尔状态只读，
没有笛卡尔命令 publisher API。

Client cache getter 返回本 client 最后一次发布或成功发送的内容，不是机器人执行
状态：

| 函数 | 输出 |
|---|---|
| `getLastMotionCommand(output)` | `MotionCommand` |
| `getLastControlGroupCommand(output)` | `ControlGroupCommand` |
| `getLastMotorConfigurationCommand(output)` | `MotorConfigurationCommand` |
| `getLastSubControllerCommand(output)` | `SubControllerCommand` |

Session 停止时会清空这些 cache。

## 命令与 Service

| 函数 | Endpoint | 租约 | 含义 |
|---|---|---|---|
| `publishMotionCommand(command)` | `MotionCommandPublisher` | `MotionCommand` | 发布完整机器人运动命令 |
| `publishControlGroupCommand(command)` | `ControlGroupCommandPublisher` | 所有目标关节资源 | 发布命名 control group 的关节命令 |
| `publishMotorConfigurationCommand(command)` | `MotorConfigurationCommandPublisher` | 所有目标关节资源 | 发布自定义电机增益/配置 |
| `setSubController(command)` | `SubControllerService` | 无 | 请求当前 Universal Controller task 应用/复位 sub-controller 配置 |
| `requestFsmState(state_id)` | `FsmService` | 无 | 请求 Aurora FSM 状态 |

由于 hardware SDK 暂未开放，电机配置临时由 Aurora SDK 提供，后续会移动到
hardware-level SDK。机器人相关的 group 名称、向量长度、mode、FSM 和能力验证
仍由 Aurora 完成。

便捷 setter 构造完整命令后委托给上表函数：

| 函数 | 构造的命令 |
|---|---|
| `setVelocity(vx, vy, yaw, duration, gait_type=0)` | `MotionCommand` 中机身坐标系 X/Y 线速度与 yaw 角速度 |
| `setStandPose(delta_height, delta_pitch, delta_yaw, duration=1.0)` | `MotionCommand` 中站立高度/pitch/yaw 偏移 |
| `resetUnicontroller()` | `reset=true` 的 `SubControllerCommand` |

当前 active task 不包含 Universal Controller 时，`setSubController()` 会失败。
命令 publish 成功不表示已经执行。

## 控制租约 API

| 函数 | 用途 |
|---|---|
| `registerLease(resources, option={})` | 注册模块的第一组完整资源 |
| `replaceLease(resources, option={})` | 原子替换当前完整资源集合 |
| `releaseLease()` | 释放当前租约；重复释放是安全的 |
| `getLease()` | 返回瞬时本地 `LeaseResult` 快照 |

`AuroraResource` 包含 `LeftLeg`、`RightLeg`、`Waist`、`Head`、
`LeftManipulator`、`RightManipulator`、`LeftEndEffector`、
`RightEndEffector` 和 `MotionCommand`。

`LeaseOption`：

| 字段 | 默认值 | 含义 |
|---|---:|---|
| `duration` | `4000ms` | 请求的租约周期；有效租约在实际周期的四分之一处自动续租（默认 grant 对应 1 秒） |
| `priority` | 未设置（`Normal`） | 可选 `Normal` 或 `Emergency` |
| `timeout` | `500ms` | register/replace 公开操作的总 deadline |

Emergency 优先级还要求 `allow_emergency_lease_priority=true`；Aurora 仍有权接受
或拒绝申请。

`LeaseResult` 包含 `operation`、`state`、`source_id`、`source_name`、
`lease_id`、`resources`、`priority`、`granted_duration`、
`remaining_duration` 和 `loss_reason`。`success()` 表示本次操作结果；
`usable()` 是更严格的发命令条件：状态为 `Active` 且剩余时间大于零。

```cpp
sdk::LeaseOption lease_option;
lease_option.duration = std::chrono::seconds(2);
lease_option.timeout = std::chrono::milliseconds(500);

auto lease = client.registerLease(
    {sdk::AuroraResource::LeftManipulator,
     sdk::AuroraResource::RightManipulator},
    lease_option);
if (!lease.usable()) {
    std::cerr << lease.operation.code << ": "
              << lease.operation.message << "\n";
}
```

Replace 与 release 期间受保护 publisher 会暂停。Replace 被明确拒绝时，如果旧
租约仍有效就恢复旧租约；结果不确定的 timeout 可能使本地租约进入 `Lost`。
租约状态不会写入磁盘。

## Callback

只能在 client stopped 时注册或替换 callback：

| 函数 | Callback 参数 |
|---|---|
| `onAuroraState(callback)` | `AuroraState` |
| `onMotionState(callback)` | `MotionState` |
| `onControlGroupState(callback)` | `ControlGroupState` |
| `onMotorConfigurationState(callback)` | `MotorConfigurationState` |
| `onEstimatorData(callback)` | `EstimatorData` |
| `onErrorCodes(callback)` | `ErrorCodes` |
| `onLeaseChanged(callback)` | `LeaseResult` |

状态 callback 在 DDS cache 更新后在线执行。它们必须有界、非阻塞、不会抛出
异常，也不能在 callback 中调用 `stop()`。租约通知由 SDK notification executor
串行执行。

## 数据类型

| 领域 | 主要类型与字段 |
|---|---|
| 通用 | `MessageHeader{send_time, frame_id, received_time}`、`OperationResult` |
| 机器人/FSM | `RobotInfo`、`StateInfo`、`AuroraState{robot_info, switch_status, current_state, available_states, authority_resources}` |
| 运动 | `MotionCommand{position, lin_velocity, attitude, ang_velocity, delta_height, gait_type, skills[8], duration}` 与对应实测/命令 `MotionState` 字段 |
| 控制组 | `ControlGroupStateEntry`、`CartesianStateEntry{link_name, pose[7], twist[6], wrench[6]}`、`JointCommand`、`ControlGroupCommand` |
| 估计器 | `BaseData`、`ContactData`、含世界系（`_W`）与机身系（`_B`）字段的 `EstimatorData` |
| 电机配置 | `MotorConfigurationGroupCommand/State`、`MotorConfigurationCommand/State`、`MotorConfigurationMode` |
| Sub-controller | `GroupOverrideParam`、`ControllerParam`、`SubControllerCommand` |
| 错误 | `ErrorCodeEntry{high32, low32}`、`ErrorCodes{source, error_codes}` |
| 租约 authority | `LeaseOption`、`LeaseResult`、`AuthorityResourceStatus` 与租约 enum |

Pose 固定数组顺序为 `[x, y, z, qx, qy, qz, qw]`。Motion position 使用世界
坐标系；除 Aurora 的机器人专用 contract 另有说明外，motion velocity 与
Cartesian state 使用机器人机身坐标系。

## 返回值与异常

只有 code `0` 时 `OperationResult::success()` 为 true。使用
`resultCodeName(code)` 和 `defaultResultMessage(code)` 查询 SDK 已知 code。

| 范围 | 所有者 |
|---:|---|
| `0` | 成功 |
| `100..199` | 本地 SDK client/transport |
| `200..299` | Aurora service，原样透传 |
| `300..319` | 租约 |
| `320..339` | Sub-controller |

重要本地失败包括 `CLIENT_NOT_RUNNING`、`ENDPOINT_DISABLED`、
`ENDPOINT_UNMATCHED`、`PUBLISH_FAILED`、`SERVICE_TIMEOUT`、
`TRANSPORT_ERROR`、`INVALID_AURORA_RESPONSE` 和 `STATE_NOT_RECEIVED`。租约
失败还会区分非法请求、资源忙、过期、操作 timeout、租约丢失和资源覆盖不足。

异常层级：

```text
std::runtime_error
└── AuroraSdkError
    ├── ConnectionError
    ├── InvalidArgumentError
    └── InvalidStateError
```

非法配置/参数捕获 `InvalidArgumentError`，生命周期误用捕获
`InvalidStateError`，transport 启停的致命失败捕获 `ConnectionError`。

## 低层 Transport facade

安装的 `transport/fourierdds_transport.hpp` 声明具体
`FourierDdsTransport`。它是集成 SDK 核心的高级 transport 接口，不是推荐的
应用 client。

| 类别 | 函数 |
|---|---|
| 构造 | `FourierDdsTransport(domain_id, options={})`、析构函数 |
| 接收 handler | `setAuroraStateHandler`、`setMotionStateHandler`、`setControlGroupStateHandler`、`setMotorConfigurationStateHandler`、`setEstimatorDataHandler`、`setErrorCodesHandler` |
| 聚合状态 | `endpointMatchStatus()` |
| Service 状态 | `isFsmServiceMatched()`、`isManageLeaseServiceMatched()`、`isSubControllerServiceMatched()` |
| Publisher 状态 | `isMotionCommandPublisherMatched()`、`isControlGroupCommandPublisherMatched()`、`isMotorConfigurationCommandPublisherMatched()` |
| Subscriber 状态 | `isAuroraStateSubscriberMatched()`、`isMotionStateSubscriberMatched()`、`isControlGroupStateSubscriberMatched()`、`isMotorConfigurationStateSubscriberMatched()`、`isEstimatorDataSubscriberMatched()`、`isErrorCodesSubscriberMatched()` |
| 无租约 service | `requestFsmState(state_id)`、`setSubController(command)` |
| 停止 | `stop()` |
| SDK 核心集成 | `publishMotionCommand`、`publishControlGroupCommand`、`publishMotorConfigurationCommand`、`manageLease` |

SDK 核心集成函数有意要求 `detail` namespace 类型。应用无法安全构造这些租约
token 或管理 request，必须使用对应的 `AuroraClient` 函数。只有维护 SDK
transport 集成时才应直接使用这个 facade。
