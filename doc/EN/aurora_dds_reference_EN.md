# Aurora DDS Reference Guide

This document provides a reference guide for the Aurora DDS API. It includes information about the data types, functions, and constants used in the Aurora DDS API.

Note: In Aurora startup config, if SerialNumber is not "null", add prefix "RobotName/SerialNumber/"to all topic names.

## Aurora Public Publisher and Subscriber Reference

### Public Publisher
#### AuroraState.idl
topic name: "aurora_state"
```
struct AuroraState {
    std_msgs::msg::Header header; 
    uint8 velocity_command_source; // 0 joystick(defalut), 1 handheld, 2 navigation
    uint8 whole_body_fsm_state;   // 0 defalut, 1 joint stand, 2 pd stand, 3~8 user a~f, 9 security, 10 usercmd, 11 upperbody usercmd
    uint8 upper_body_fsm_state; // 0 defalut, 1 lower-upper linkage, 2 remote ctrl
    boolean allow_upper_body_override; // False no, True yes
};
```
#### BaseData.idl
topic name: "base_data_pub"
```
struct BaseData {
    std_msgs::msg::Header header;
    sequence<double> quat_xyzw; //base2world
    sequence<double> quat_wxyz; //base2world
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
topic name: "contact_data_pub"
```
struct ContactData {
    std_msgs::msg::Header header;
    sequence<double> contact_force; // feet contact force 6Dx2
    sequence<double> contact_prob; // 0~1
};
```
#### MotionControlState.idl
topic name: "robot_control_group_state"
```
struct RobotControlGroupState {
    std_msgs::msg::Header header;
    sequence<uint8> group_motion_state;  // 8D, 0 for idle, 1 for move, 2 for stop 3 for error
    sequence<ControlGroupState> group_state; // 8D, state for each group
    uint8 stable_level; // degree of robot stability, 0 for unstable, 1 for stable
    sequence<std_msgs::msg::BaseDataType> ex_data; // extra data if needed
};

struct ControlGroupState {
    string group_name; // left_leg, right_leg, waist, head, left_manipulator, right_manipulator, left_hand, right_hand
    sequence<double> joint_position;
    sequence<double> joint_velocity;
    sequence<double> joint_effort;
    sequence<double> motor_position;
    sequence<double> motor_velocity;
    sequence<double> motor_effort;
};

```
topic name: "robot_cartesian_state"
```
struct RobotCartesianState {
    std_msgs::msg::Header header;
    uint8 coordinate_system; // 0 for base_link, 1 for world
    sequence<uint8> group_motion_state; // 2D, 0 for idle, 1 for move, 2 for stop
    sequence<CartesianState> cartesian_state; //2D, state for each manipulator
    uint8 stable_level; // degree of robot stability, 0 for unstable, 1 for stable
    sequence<std_msgs::msg::BaseDataType> ex_data; // extra data if needed
};

struct CartesianState {
    string group_name;  //  left_manipulator, right_manipulator
    sequence<double> pose; // x, y, z, qx, qy, qz, qw
    sequence<double> twist; // vx, vy, vz, wx, wy, wz
    sequence<double> wrench; // fx, fy, fz, tx, ty, tz
};
```

### Public Subscriber
#### AuroraCmd.idl

topic name: "velocity_source_cmd"
```
struct VelocitySourceCmd {
    std_msgs::msg::Header header;
    uint8 desired_source; // 0 joystick, 1 handheld, 2 navigation
};
```
topic name: "whole_body_fsm_state_change_cmd"
```
struct WholeBodyFsmStateChangeCmd {
    std_msgs::msg::Header header;
    uint8 desired_state;     // 0 defalut, 1 joint stand, 2 pd stand, 3~8 user a~f, 9 security, 10 usercmd, 11 upperbody usercmd
};
```

## Pd Stand Task Reference
### Pd Stand Publisher

#### AuroraState.idl

topic name "robot_stand_pose_state"
```
struct RobotStandPoseState {     // RobotStand state 
    std_msgs::msg::Header header;
    double delta_z;             //  z axis height
    double delta_pitch;         // pitch value 
    double delta_yaw;           // yaw value
    uint8 stable_level;         
};
```

### PdStand Subscriber
#### AuroraCmd.idl

topic name "robot_stand_pose_cmd"
```
struct RobotStandPoseCmd {
    std_msgs::msg::Header header;
    double delta_z;
    double delta_pitch;
    double delta_yaw;
};
```
#### MotionControlCmd.idl

topic name "robot_control_group_cmd"

```
struct RobotControlGroupCmd {
    std_msgs::msg::Header header;
    sequence<ControlGroupCmd> group_cmd;
    sequence<std_msgs::msg::BaseDataType> ex_data; // extra data if needed
};

struct ControlGroupCmd {
    string group_name; // left_leg, right_leg, waist, head, left_manipulator, right_manipulator, left_hand, right_hand
    uint8 group_motion_state;  // 1 for move, 2 for stop, if stop ignore the rest fields
    uint8 control_type; // 0 for control joint, 1 for control motor
    uint8 motor_mode; // 0 for control position, 1 for control effort
    sequence<double> position; // for position control, this item is essential, for effort control, it will be ignore .
    sequence<double> velocity; // for position control, this is essential, for effort control this will be ignored .
    sequence<double> effort; // for position control, this is essential, for effort control this will be essential .
};

```

## UserCmd Task Reference
### UserCmd Subscriber
#### MotionControlCmd.idl

topic name "robot_control_group_cmd"

```
struct RobotControlGroupCmd {
    std_msgs::msg::Header header;
    sequence<ControlGroupCmd> group_cmd;
    sequence<std_msgs::msg::BaseDataType> ex_data; // extra data if needed
};

struct ControlGroupCmd {
    string group_name; // left_leg, right_leg, waist, head, left_manipulator, right_manipulator, left_hand, right_hand
    sequence<double> position; // for position control, this item is essential, for effort control, it will be ignore .
    sequence<double> velocity; // for position control, this is essential, for effort control this will be ignored .
    sequence<double> effort; // for position control, this is essential, for effort control this will be essential .
};

```
#### MotorCfgCmd.idl

topic name "robot_motor_cfg_group_cmd"
```
struct RobotMotorCfgGroupCmd {
    std_msgs::msg::Header header;
    sequence<MotorCfgGroupCmd> group_cmd;
    sequence<std_msgs::msg::BaseDataType> ex_data; // extra data if needed
};

struct MotorCfgGroupCmd {
    string group_name;
    sequence<double> pd_kp;            // pd control p
    sequence<double> pd_kd;            // pd control d
};

```

## UpperBody UserCmd Task Reference

Same as UserCmd Task Reference.

## LowerBodyCpgTask Reference
### LowerBodyCpgTask Subscriber
#### AuroraCmd.idl
topic name "velocity_cmd"
```
struct VelocityCmd {
    std_msgs::msg::Header header;
    double vx;
    double vy;
    double yaw;
};
```
## UpperBodyStateManagerTask Reference
### UpperBodyStateManagerTask Subscriber
#### AuroraCmd.idl
topic name "upper_body_fsm_state_change_cmd"
```
struct UpperBodyFsmStateChangeCmd {
    std_msgs::msg::Header header;
    uint8 desired_state;     // 0 defalut, 1 lower-upper linkage, 2 remote ctrl
};
```
#### MotionControlCmd.idl

topic name "robot_control_group_cmd"

```
struct RobotControlGroupCmd {
    std_msgs::msg::Header header;
    sequence<ControlGroupCmd> group_cmd;
    sequence<std_msgs::msg::BaseDataType> ex_data; // extra data if needed
};

struct ControlGroupCmd {
    string group_name; // left_leg, right_leg, waist, head, left_manipulator, right_manipulator, left_hand, right_hand
    sequence<double> position; // for position control, this item is essential, for effort control, it will be ignore .
    sequence<double> velocity; // for position control, this is essential, for effort control this will be ignored .
    sequence<double> effort; // for position control, this is essential, for effort control this will be essential .
};

```
