# GR2 机器人控制器参考

这是 fourier-aurora 和扩展包中提供的控制器参考。控制器在 FSM 状态机中以 **任务（Task）** 的形式提供。Aurora 运行一个 FSM 状态机，每个状态对应某些任务（控制器）。你可以使用手柄或通过 DDS 接口发送命令在这些状态之间切换。每个任务都有其输入和输出。

Aurora 提供了一组可用于控制机器人运动的控制器。一些通用控制器在 `fourier-aurora` 包中提供，而其他控制器则根据机器人配置在扩展包中提供。对于 GR-2 机器人，请使用 `fourier-aurora-gr2`。

关于 DDS 参考，请参阅 [DDS 接口参考](../aurora_dds_reference_EN.md)。

## 通用控制器

状态名 | 任务名 | 手柄映射 | DDS 映射 | 描述
-------|---------|----------|----------|-----
Default | DefaultTask | LB+RB | 0 | 默认状态，不执行任何操作，用作 FSM 启动后的默认状态。
SecurityProtection | SecurityProtectionTask | LT+RT | 9 | 安全状态，紧急情况下停止所有执行器。在退出该状态时会重新激活执行器。
JointStand | JointStandTask | LB+A | 1 | 所有执行器移动到零位。用于检查所有关节是否正常工作并完成零点校准。
PdStand | PdStandTask | LB+B | 2 | 机器人站在平面上，支持上半身关节（腰部、手臂和头部）的外部关节位置命令，以及包括基座高度、俯仰和偏航的站姿调整。
UserCmd | UserCmdTask | - | 10 | 执行外部关节控制和配置命令。
UpperBodyUserCmd | UpperBodyUserCmdTask | - | 11 | 执行上半身关节（腰部、手臂和头部）的外部关节控制和配置命令。

### PdStand 任务规范

此任务用于让机器人站在平面上以进行进一步控制。切换到 PdStand 状态后，如果机器人处于悬挂状态，它将首先弯曲膝盖进行站立准备。然后可以将机器人垂直放置在平面上（z 轴上无外力，例如悬挂绳）。当稳定等级达到 100 时，机器人将进入站立状态，用户可以使用外部关节位置命令控制上半身关节（腰部、手臂和头部），以及基座高度、俯仰和偏航的相对（相对于初始姿态）增量命令。

**输入：**

- 上半身关节（腰部、手臂和头部）的外部关节位置命令。
  - 单位：弧度
  - 范围：根据 urdf 关节限制
  - 注意：仅在机器人处于站立状态时生效。
  - 参考：[机器人控制组命令](../aurora_dds_reference_EN.md#robot-control-group-command)

- 基座高度、俯仰和偏航的相对增量命令（相对于初始姿态）。
  - 单位：高度为米，俯仰和偏航为弧度
  - 范围（gr2）：delta_z [-0.2, 0.025], delta_pitch [-0.3, 0.45], delta_yaw [-0.5, 0.5]
  - 注意：仅在机器人处于站立状态时生效。
  - 参考：[机器人站姿命令](../aurora_dds_reference_EN.md#robot-stand-pose-command)

**输出：**

- 基座高度、俯仰和偏航的相对状态（相对于初始姿态）。
  - 单位：高度为米，俯仰和偏航为弧度
  - 范围：delta_z [-0.2, 0.025], delta_pitch [-0.3, 0.45], delta_yaw [-0.5, 0.5]
  - 注意：仅在机器人处于站立状态时生效。
  - 参考：[机器人站姿状态](../aurora_dds_reference_EN.md#robot-stand-pose-state)

- 稳定等级
  - 单位：-
  - 范围：真实机器人 [0,110]，仿真 [0,15]
  - 注意：当稳定等级低于 100 时，真实机器人将进入悬挂状态。
  - 参考：[机器人站姿状态](../aurora_dds_reference_EN.md#robot-stand-pose-state)

### UserCmd 任务规范

此任务用于执行外部关节控制和配置命令。切换到 UserCmd 状态后，用户可以发送所有关节的外部关节位置命令，以及外部配置命令（如 PD 参数）。机器人将执行这些命令并相应更新状态。

**输入：**

- 所有关节的外部关节位置命令。
  - 单位：位置参考为弧度，速度参考为弧度/秒，力矩前馈为牛米
  - 范围：根据 urdf 关节限制
  - 参考：[机器人控制组命令](../aurora_dds_reference_EN.md#robot-control-group-command)

- 外部配置命令（如 PD 参数）。
  - 单位：-
  - 范围：-
  - 参考：[机器人电机配置组命令](../aurora_dds_reference_EN.md#robot-motor-config-group-command)

### UpperBodyUserCmd 任务规范

此任务用于执行上半身关节（腰部、手臂和头部）的外部关节控制和配置命令。切换到 UpperBodyUserCmd 状态后，腿部执行器将被禁用，直到退出该状态。

**输入：**

- 上半身关节（腰部、手臂和头部）的外部关节位置命令。
  - 单位：位置参考为弧度，速度参考为弧度/秒，力矩前馈为牛米
  - 范围：根据 urdf 关节限制
  - 参考：[机器人控制组命令](../aurora_dds_reference_EN.md#robot-control-group-command)

- 外部配置命令（如 PD 参数）。
  - 单位：-
  - 范围：-
  - 参考：[机器人电机配置组命令](../aurora_dds_reference_EN.md#robot-motor-config-group-command)

## GR-2 控制器

状态名 | 任务名 | 手柄映射 | DDS 映射 | 描述
-------|---------|----------|----------|-----
UserController_A | LowerBodyCpgRLTask UpperBodyStateManagerTask | RB+A | 3 | 直腿 RL 策略行走，接收外部速度命令 (vx, vy, vyaw)，支持上半身关节（腰部、手臂、头部和手）的外部关节控制命令
<!-- UserController_B | LowerBodyWBCLowerTask UpperBodyStateManagerTask| LB+B | 4 | 敏捷 RL 策略行走，接收外部速度命令 (vx, vy, vyaw) 和站姿调整（包括基座高度和俯仰），支持上半身关节（腰部、手臂、头部和手）的外部关节控制命令 -->

### LowerBodyCpgRLTask 任务规范

此任务用于通过策略控制下半身关节（髋、膝和踝），使机器人能够行走。建议先切换到 PdStand 状态以确保机器人站在平面上。切换到 LowerBodyCpgRLTask 后，机器人将执行 CPG 策略来控制下半身关节。

**输入：**

- 速度命令 (vx, vy, vyaw)。
  - 单位：vx 和 vy 为 m/s，vyaw 为弧度/秒
  - 范围：vx [-0.5, 0.75], vy [-0.5, 0.5], vyaw [-1.0, 1.0]
  - 参考：[速度命令](../aurora_dds_reference_EN.md#velocity-command)

### UpperBodyStateManagerTask 任务规范

此任务用于管理上半身关节（腰部、手臂和头部）。通常与下半身控制器配合使用。切换到 UpperBodyStateManagerTask 状态后，机器人将执行一个状态机来管理上半身关节。初始状态为 "default"。用户可以切换到其他状态以使手臂摆动，或接收上半身关节（腰部、手臂和头部）的外部关节控制命令。**现在建议使用 PdStand 来进行稳定的上半身控制。**

**输入：**

- 上半身状态切换命令。
  - 单位：-
  - 范围：0 默认，1 下-上联动，2 远程控制
  - 参考：[上半身 FSM 状态切换命令](../aurora_dds_reference_EN.md#upper-body-fsm-state-change-command)

- 上半身关节（腰部、手臂和头部）的外部关节位置命令。
  - 单位：位置参考为弧度，速度参考为弧度/秒，力矩前馈为牛米
  - 范围：根据 urdf 关节限制
  - 注意：仅在上半身状态为 "远程控制" 时生效。
  - 参考：[机器人控制组命令](../aurora_dds_reference_EN.md#robot-control-group-command)
