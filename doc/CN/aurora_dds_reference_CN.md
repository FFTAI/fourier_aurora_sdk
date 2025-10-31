
# Aurora DDS 参考指南

本文件为 Aurora DDS API 的参考指南，包含 Aurora DDS API 中使用的数据类型、函数和常量的信息。

注意：在 Aurora 启动配置中，如果 SerialNumber 不是 "null"，则在所有主题（topic）名称前添加前缀 `RobotName/SerialNumber/`。

## Aurora 公共发布者与订阅者参考

### 公共发布者

#### Aurora 状态

**主题名称（Topic Name）：** `aurora_state`  
**类型名称（Type Name）：** `fourier_msgs::msg::AuroraState`  
**定义位置（Locate in）：** `AuroraState.idl`  
**可用任务（Available task）：** all tasks

发布 Aurora 有限状态机（FSM）的当前状态，包括速度指令来源、整机与上身 FSM 的当前状态，以及是否允许上身状态切换。

```idl
struct AuroraState {
    std_msgs::msg::Header header; 
    uint8 velocity_command_source; // 0 操纵杆（默认），1 手持，2 导航
    uint8 whole_body_fsm_state;   // 0 默认，1 关节站立，2 PD 站立，3~8 用户 a~f，9 安全，10 usercmd，11 上身 usercmd
    uint8 upper_body_fsm_state; // 0 默认，1 下-上 链接，2 远程控制
    boolean allow_upper_body_override; // False 表示不允许，True 表示允许
};
```

#### 基座数据

**主题名称：** `base_data_pub`  
**类型名称：** `fourier_msgs::msg::BaseData`  
**定义位置：** `BaseData.idl`  
**可用任务：** all tasks

发布机器人基座（base）在 world 与 base 坐标系下的估计位姿、方向与速度，包含四元数表示、滚转-俯仰-偏航（RPY）角、角速度、加速度、线速度与线位置等信息。

```idl
struct BaseData {
    std_msgs::msg::Header header;
    sequence<double> quat_xyzw; // base -> world 四元数（x, y, z, w）
    sequence<double> quat_wxyz; // base -> world 四元数（w, x, y, z）
    sequence<double> rpy;       // 滚转-俯仰-偏航
    sequence<double> omega_W;   // 世界坐标系下角速度
    sequence<double> acc_W;     // 世界坐标系下加速度
    sequence<double> omega_B;   // 机体坐标系下角速度
    sequence<double> acc_B;     // 机体坐标系下加速度
    sequence<double> vel_W;     // 世界坐标系下线速度
    sequence<double> pos_W;     // 世界坐标系下位置
    sequence<double> vel_B;     // 机体坐标系下线速度
};
```

#### 接触数据

**主题名称：** `contact_data_pub`  
**类型名称：** `fourier_msgs::msg::ContactData`  
**定义位置：** `ContactData.idl`  
**可用任务：** all tasks

发布机器人两只脚的估计接触力和接触概率。接触力数组提供每只脚的 6 维力/力矩数据，概率数组表示每只脚与地面接触的可能性（范围 0 到 1）。

```idl
struct ContactData {
    std_msgs::msg::Header header;
    sequence<double> contact_force; // 两只脚的接触力，6D x 2
    sequence<double> contact_prob;  // 接触概率，范围 0~1
};
```

#### 机器人关节组状态

**主题名称：** `robot_control_group_state`  
**类型名称：** `fourier_msgs::msg::RobotControlGroupState`  
**定义位置：** `MotionControlState.idl`  
**可用任务：** all tasks

发布机器人按控制组（如腿、腰、机械臂等）组织的每个关节状态。对每个组，包含关节位置、速度、力矩（effort）以及当前运动状态（空闲、移动、停止、错误）。

```idl
struct RobotControlGroupState {
    std_msgs::msg::Header header;
    sequence<uint8> group_motion_state;  // 8 维：0 表示空闲，1 表示移动，2 表示停止，3 表示错误
    sequence<ControlGroupState> group_state; // 8 维：每个组的状态
    uint8 stable_level; // 暂未使用
    sequence<std_msgs::msg::BaseDataType> ex_data; // 额外数据（如需要）
};

struct ControlGroupState {
    string group_name; // left_leg, right_leg, waist, head, left_manipulator, right_manipulator, left_hand, right_hand
    sequence<double> joint_position;
    sequence<double> joint_velocity;
    sequence<double> joint_effort;
    sequence<double> motor_position; // 未使用
    sequence<double> motor_velocity; // 未使用
    sequence<double> motor_effort;   // 未使用
};
```

#### 机器人笛卡尔（末端）状态

**主题名称：** `robot_cartesian_state`  
**类型名称：** `fourier_msgs::msg::RobotCartesianState`  
**定义位置：** `MotionControlState.idl`  
**可用任务：** all tasks

发布每个机械臂（如左/右臂）在基座或世界坐标系下的笛卡尔末端状态。对每个机械臂，包含位姿（位置与朝向）、速度（线速度与角速度）以及力/力矩（wrench）。

```idl
struct RobotCartesianState {
    std_msgs::msg::Header header;
    uint8 coordinate_system; // 0 表示 base_link，1 表示 world
    sequence<uint8> group_motion_state; // 2 维：0 表示空闲，1 表示移动，2 表示停止
    sequence<CartesianState> cartesian_state; // 2 维：每个机械臂的状态
    uint8 stable_level; // 机器人稳定等级，0 表示不稳定，1 表示稳定
    sequence<std_msgs::msg::BaseDataType> ex_data; // 额外数据（如需要）
};

struct CartesianState {
    string group_name;  // left_manipulator, right_manipulator
    sequence<double> pose; // x, y, z, qx, qy, qz, qw
    sequence<double> twist; // vx, vy, vz, wx, wy, wz
    sequence<double> wrench; // fx, fy, fz, tx, ty, tz
};
```

#### 机器人站立位姿状态

**主题名称：** `robot_stand_pose_state`  
**类型名称：** `fourier_msgs::msg::RobotStandPoseState`  
**定义位置：** `AuroraState.idl`  
**可用任务：** LowerBodyWBCLower, PdStand（在 stance 阶段）

发布机器人基座相对于预定义初始姿态的当前估计姿态变化，包含高度、俯仰和偏航的变化，以及稳定等级指示。

```idl
struct RobotStandPoseState {     // RobotStand 状态 
    std_msgs::msg::Header header;
    double delta_z;             // z 轴高度变化
    double delta_pitch;         // 俯仰变化值
    double delta_yaw;           // 偏航变化值
    uint8 stable_level;         // 稳定等级
};
```

### 公共订阅者

#### 全身FSM状态切换指令

**主题名称：** `whole_body_fsm_state_change_cmd`  
**类型名称：** `fourier_msgs::msg::WholeBodyFsmStateChangeCmd`  
**定义位置：** `AuroraCmd.idl`  
**可用任务：** all tasks

接收外部指令以将 Aurora 的整机 FSM 状态切换到指定目标状态。这允许外部控制器或用户界面触发状态转换，例如在默认、关节站立、PD 站立、用户自定义状态、安全模式和指令模式之间切换。

```idl
struct WholeBodyFsmStateChangeCmd {
    std_msgs::msg::Header header;
    uint8 desired_state;     // 0 默认，1 关节站立，2 PD 站立，3~8 用户 a~f，9 安全，10 usercmd，11 上身 usercmd
};
```

#### 上身FSM状态切换指令

**主题名称：** `upper_body_fsm_state_change_cmd`  
**类型名称：** `fourier_msgs::msg::UpperBodyFsmStateChangeCmd`  
**定义位置：** `AuroraCmd.idl`  
**可用任务：** UpperBodyStateManager

接收指令以将上身 FSM 状态切换到指定目标状态，使外部系统能够独立控制上身，例如在默认、下-上 链接或远程控制模式之间切换。

```idl
struct UpperBodyFsmStateChangeCmd {
    std_msgs::msg::Header header;
    uint8 desired_state;     // 0 默认，1 下-上 链接，2 远程控制
};
```

#### 速度来源指令

**主题名称：** `velocity_source_cmd`  
**类型名称：** `fourier_msgs::msg::VelocitySourceCmd`  
**定义位置：** `AuroraCmd.idl`  
**可用任务：** all tasks

接收指令以设置速度指令的来源，允许在操纵杆、手持设备或导航模块之间动态切换。使用DDS通讯请将速度指令来源设置为2（导航）。

```idl
struct VelocitySourceCmd {
    std_msgs::msg::Header header;
    uint8 desired_source; // 0 操纵杆，1 手持，2 导航
};
```

#### 速度指令

**主题名称：** `velocity_cmd`  
**类型名称：** `fourier_msgs::msg::VelocityCmd`  
**定义位置：** `AuroraCmd.idl`  
**可用任务：** LowerBodyCpgRL, LowerBodyWBCLower

接收用于行走任务的速度指令，指定期望的线速度（vx、vy）和偏航角速度。

```idl
struct VelocityCmd {
    std_msgs::msg::Header header;
    double vx;
    double vy;
    double yaw;
};
```

#### 机器人控制组指令

**主题名称：** `robot_control_group_cmd`  
**类型名称：** `fourier_msgs::msg::RobotControlGroupCmd`  
**定义位置：** `MotionControlCmd.idl`  
**可用任务：** UserCmd, UpperBodyUserCmd, UpperBodyTeleTask, PdStand（在 stance 阶段）

接收按控制组组织的关节直接控制指令。对于每个组，指定期望的运动状态、控制类型（控制关节或控制电机）、电机模式（位置或力矩/努力）、以及目标位置、速度和力矩值。

```idl
struct ControlGroupCmd {
    string group_name; // left_leg, right_leg, waist, head, left_manipulator, right_manipulator, left_hand, right_hand
    uint8 group_motion_state;  // 1 表示移动，2 表示停止；如果为停止，则忽略其余字段
    uint8 control_type; // 0 表示控制关节，1 表示控制电机
    uint8 motor_mode; // 0 表示位置控制，1 表示力矩/努力控制
    sequence<double> position; // 位置控制时此字段为必需；在力矩控制下会被忽略。
    sequence<double> velocity; // 位置控制时此字段为必需；在力矩控制下会被忽略。
    sequence<double> effort; // 位置控制时此字段为必需；力矩控制时此字段为必需。
};

struct RobotControlGroupCmd {
    std_msgs::msg::Header header;
    sequence<ControlGroupCmd> group_cmd;
    sequence<std_msgs::msg::BaseDataType> ex_data; // 额外数据（如需要）
};
```

#### 机器人电机配置指令

**主题名称：** `robot_motor_cfg_group_cmd`  
**类型名称：** `fourier_msgs::msg::RobotMotorCfgGroupCmd`  
**定义位置：** `MotorCfgCmd.idl`  
**可用任务：** UserCmd, UpperBodyUserCmd

接收用于执行器控制器的配置指令，允许为每个控制组设置比例（P）与微分（D）增益。

```idl
struct RobotMotorCfgGroupCmd {
    std_msgs::msg::Header header;
    sequence<MotorCfgGroupCmd> group_cmd;
    sequence<std_msgs::msg::BaseDataType> ex_data; // 额外数据（如需要）
};

struct MotorCfgGroupCmd {
    string group_name;
    sequence<double> pd_kp;            // PD 控制的 P 增益
    sequence<double> pd_kd;            // PD 控制的 D 增益
};
```

#### 机器人站立位姿指令

**主题名称：** `robot_stand_pose_cmd`  
**类型名称：** `fourier_msgs::msg::RobotStandPoseState`  
**定义位置：** `AuroraCmd.idl`  
**可用任务：** LowerBodyWBCLower, PdStand（在 stance 阶段）

接收用于调整机器人站立时基座高度与朝向（俯仰与偏航）相对于初始位姿的指令。

```idl
struct RobotStandPoseState {     // RobotStand 状态 
    std_msgs::msg::Header header;
    double delta_z;             // z 轴高度变化
    double delta_pitch;         // 俯仰变化值
    double delta_yaw;           // 偏航变化值
    uint8 stable_level;         
};
```

