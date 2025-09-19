# Aurora DDS API 参考指南

本文档提供Aurora DDS API的参考指南，包含API中使用的数据类型、函数和常量的详细信息。

注意：在Aurora启动配置中，如果SerialNumber不是"null"，需要在所有主题名称前添加前缀"RobotName/SerialNumber/"。

## 公共发布者和订阅者参考

### 公共发布者
#### AuroraState.idl
主题名称: "aurora_state"
```
struct AuroraState {
    std_msgs::msg::Header header; 
    uint8 velocity_command_source; // 0 摇杆(默认), 1 手持设备, 2 导航
    uint8 whole_body_fsm_state;   // 0 默认, 1 关节站立, 2 PD站立, 3~8 用户a~f, 9 安全模式, 10 用户命令, 11 上半身用户命令
    uint8 upper_body_fsm_state; // 0 默认, 1 下半身-上半身联动, 2 远程控制
    boolean allow_upper_body_override; // False 不允许, True 允许
};
```
#### BaseData.idl
主题名称: "base_data_pub"
```
struct BaseData {
    std_msgs::msg::Header header;
    sequence<double> quat_xyzw; //基座到世界坐标系
    sequence<double> quat_wxyz; //基座到世界坐标系
    sequence<double> rpy;
    sequence<double> omega_W;
    sequence<double> acc_W;
    sequence<double> omega_B;
    sequence<double> acc_B;
    sequence<double> vel_W;
    sequence<double> pos_W;
    sequence<double> vel_B;
};
```
#### ContactData.idl
主题名称: "contact_data_pub"
```
struct ContactData {
    std_msgs::msg::Header header;
    sequence<double> contact_force; // 足部接触力 6Dx2
    sequence<double> contact_prob; // 0~1
};
```
#### MotionControlState.idl
主题名称: "robot_control_group_state"
```
struct RobotControlGroupState {
    std_msgs::msg::Header header;
    sequence<uint8> group_motion_state;  // 8D, 0 空闲, 1 移动, 2 停止, 3 错误
    sequence<ControlGroupState> group_state; // 8D, 每个控制组的状态
    uint8 stable_level; // 机器人稳定程度, 0 不稳定, 1 稳定
    sequence<std_msgs::msg::BaseDataType> ex_data; // 额外数据(如果需要)
};

struct ControlGroupState {
    string group_name; // 左腿,右腿,腰部,头部,左机械臂,右机械臂,左手,右手
    sequence<double> joint_position;
    sequence<double> joint_velocity;
    sequence<double> joint_effort;
    sequence<double> motor_position;
    sequence<double> motor_velocity;
    sequence<double> motor_effort;
};
```
主题名称: "robot_cartesian_state"
```
struct RobotCartesianState {
    std_msgs::msg::Header header;
    uint8 coordinate_system; // 0 基座坐标系, 1 世界坐标系
    sequence<uint8> group_motion_state; // 2D, 0 空闲, 1 移动, 2 停止
    sequence<CartesianState> cartesian_state; //2D, 每个机械臂的状态
    uint8 stable_level; // 机器人稳定程度, 0 不稳定, 1 稳定
    sequence<std_msgs::msg::BaseDataType> ex_data; // 额外数据(如果需要)
};

struct CartesianState {
    string group_name;  // 左机械臂,右机械臂
    sequence<double> pose; // x, y, z, qx, qy, qz, qw
    sequence<double> twist; // vx, vy, vz, wx, wy, wz
    sequence<double> wrench; // fx, fy, fz, tx, ty, tz
};
```

### 公共订阅者
#### AuroraCmd.idl
主题名称: "velocity_source_cmd"
```
struct VelocitySourceCmd {
    std_msgs::msg::Header header;
    uint8 desired_source; // 0 摇杆, 1 手持设备, 2 导航
};
```
主题名称: "whole_body_fsm_state_change_cmd"
```
struct WholeBodyFsmStateChangeCmd {
    std_msgs::msg::Header header;
    uint8 desired_state; // 0 默认, 1 关节站立, 2 PD站立, 3~8 用户a~f, 9 安全模式, 10 用户命令, 11 上半身用户命令
};
```

## PD站立任务参考
### PD站立发布者
#### AuroraState.idl
主题名称 "robot_stand_pose_state"
```
struct RobotStandPoseState {     // 机器人站立状态 
    std_msgs::msg::Header header;
    double delta_z;             // z轴高度
    double delta_pitch;         // 俯仰值
    double delta_yaw;           // 偏航值
    uint8 stable_level;         // 稳定程度
};
```

### PD站立订阅者
#### AuroraCmd.idl
主题名称 "robot_stand_pose_cmd"
```
struct RobotStandPoseCmd {
    std_msgs::msg::Header header;
    double desired_z;          // 期望高度
    double desired_pitch;      // 期望俯仰角
    double desired_yaw;         // 期望偏航角
};
```
#### MotionControlCmd.idl
主题名称 "robot_control_group_cmd"
```
struct RobotControlGroupCmd {
    std_msgs::msg::Header header;
    sequence<ControlGroupCmd> groups_cmd; // 控制组命令
    uint8 control_mode; // 0 位置控制, 1 速度控制, 2 力矩控制
};

struct ControlGroupCmd {
    string group_name; // 控制组名称
    sequence<double> position; // 期望位置
    sequence<double> velocity; // 期望速度
    sequence<double> effort;   // 期望力矩
    sequence<double> kp;       // 比例增益
    sequence<double> kd;       // 微分增益
};
```

## 用户命令任务参考
### 用户命令订阅者
#### MotionControlCmd.idl
主题名称 "robot_control_group_cmd"
```
struct RobotControlGroupCmd {
    std_msgs::msg::Header header;
    sequence<ControlGroupCmd> groups_cmd; // 控制组命令
    uint8 control_mode; // 0 位置控制, 1 速度控制, 2 力矩控制
};

struct ControlGroupCmd {
    string group_name; // 控制组名称
    sequence<double> position; // 期望位置
    sequence<double> velocity; // 期望速度
    sequence<double> effort;   // 期望力矩
    sequence<double> kp;       // 比例增益
    sequence<double> kd;       // 微分增益
};
```
#### MotorCfgCmd.idl
主题名称 "robot_motor_cfg_group_cmd"
```
struct RobotMotorCfgGroupCmd {
    std_msgs::msg::Header header;
    sequence<MotorCfgGroupCmd> groups_cmd; // 电机配置命令
};

struct MotorCfgGroupCmd {
    string group_name; // 控制组名称
    sequence<uint8> motor_id; // 电机ID
    sequence<uint8> motor_mode; // 电机模式
    sequence<double> motor_param; // 电机参数
};
```

## 上半身用户命令任务参考
与用户命令任务参考相同。

## 下半身CPG任务参考
### 下半身CPG任务订阅者
#### AuroraCmd.idl
主题名称 "velocity_cmd"
```
struct VelocityCmd {
    std_msgs::msg::Header header;
    double linear_x;  // 线速度x
    double linear_y;  // 线速度y
    double angular_z; // 角速度z
};
```

## 上半身状态管理任务参考
### 上半身状态管理任务订阅者
#### AuroraCmd.idl
主题名称 "upper_body_fsm_state_change_cmd"
```
struct UpperBodyFsmStateChangeCmd {
    std_msgs::msg::Header header;
    uint8 desired_state; // 0 默认, 1 下半身-上半身联动, 2 远程控制
};
```
#### MotionControlCmd.idl
主题名称 "robot_control_group_cmd"
```
struct RobotControlGroupCmd {
    std_msgs::msg::Header header;
    sequence<ControlGroupCmd> groups_cmd; // 控制组命令
    uint8 control_mode; // 0 位置控制, 1 速度控制, 2 力矩控制
};

struct ControlGroupCmd {
    string group_name; // 控制组名称
    sequence<double> position; // 期望位置
    sequence<double> velocity; // 期望速度
    sequence<double> effort;   // 期望力矩
    sequence<double> kp;       // 比例增益
    sequence<double> kd;       // 微分增益
};
```
