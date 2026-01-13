# Aurora DDS Reference Guide

This document provides a reference guide for the Aurora DDS API. It includes information about the data types, functions, and constants used in the Aurora DDS API.

Note: In Aurora startup config, if SerialNumber is not "null", add prefix "RobotName/SerialNumber/"to all topic names.

## Aurora Public Publisher and Subscriber Reference

### Public Publisher

#### Aurora State

**Topic Name:** `aurora_state`  
**Type Name:** `fourier_msgs::msg::AuroraState`  
**Locate in:** `AuroraState.idl`  
**Available task:** all tasks

Publishes the current state of Aurora's finite state machine (FSM), including the source of velocity commands, the current state of the whole body and upper body FSMs, and whether upper body override is allowed.

```idl
struct AuroraState {
    std_msgs::msg::Header header; 
    uint8 velocity_command_source; // 0 joystick(default), 1 handheld, 2 navigation
    uint8 whole_body_fsm_state;   // 0 default, 1 joint stand, 2 pd stand, 3~8 user a~f, 9 security, 10 usercmd, 11 upperbody usercmd
    uint8 upper_body_fsm_state; // 0 default, 1 lower-upper linkage, 2 remote ctrl
    boolean allow_upper_body_override; // False no, True yes
};
```

#### Base Data

**Topic Name:** `base_data_pub`  
**Type Name:** `fourier_msgs::msg::BaseData`  
**Locate in:** `BaseData.idl`  
**Available task:** all tasks

Publishes the estimated pose, orientation, and velocity of the robot's base in both the world and base frames. This includes quaternion representations, roll-pitch-yaw (RPY) angles, angular velocities, accelerations, and linear velocities/positions.

```idl
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

#### Contact Data

**Topic Name:** `contact_data_pub`  
**Type Name:** `fourier_msgs::msg::ContactData`  
**Locate in:** `ContactData.idl`  
**Available task:** all tasks

Publishes the estimated contact forces and contact probabilities for both feet of the robot. The contact force array provides 6D force/torque data for each foot, while the probability array indicates the likelihood of each foot being in contact with the ground (ranging from 0 to 1).

```idl
struct ContactData {
    std_msgs::msg::Header header;
    sequence<double> contact_force; // feet contact force 6Dx2
    sequence<double> contact_prob; // 0~1
};
```

#### Robot Control Group State

**Topic Name:** `robot_control_group_state`  
**Type Name:** `fourier_msgs::msg::RobotControlGroupState`  
**Locate in:** `MotionControlState.idl`  
**Available task:** all tasks

Publishes the state of each joint in the robot, organized by control groups (such as legs, waist, arms, etc.). For each group, it includes joint positions, velocities, and efforts, as well as the current motion state (idle, move, stop, error).

```idl
struct RobotControlGroupState {
    std_msgs::msg::Header header;
    sequence<uint8> group_motion_state;  // 8D, 0 for idle, 1 for move, 2 for stop 3 for error
    sequence<ControlGroupState> group_state; // 8D, state for each group
    uint8 stable_level; // not in use
    sequence<std_msgs::msg::BaseDataType> ex_data; // extra data if needed
};

struct ControlGroupState {
    string group_name; // left_leg, right_leg, waist, head, left_manipulator, right_manipulator, left_hand, right_hand
    sequence<double> joint_position;
    sequence<double> joint_velocity;
    sequence<double> joint_effort;
    sequence<double> motor_position; // not in use
    sequence<double> motor_velocity; // not in use
    sequence<double> motor_effort; // not in use
};
```

#### Robot Cartesian State

**Topic Name:** `robot_cartesian_state`  
**Type Name:** `fourier_msgs::msg::RobotCartesianState`  
**Locate in:** `MotionControlState.idl`  
**Available task:** all tasks

Publishes the Cartesian (end-effector) state for each manipulator (e.g., arms) in the robot, relative to the base frame or world frame. For each manipulator, it includes pose (position and orientation), twist (linear and angular velocity), and wrench (force and torque).

```idl
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

#### Robot Stand Pose State

**Topic Name:** `robot_stand_pose_state`  
**Type Name:** `fourier_msgs::msg::RobotStandPoseState`  
**Locate in:** `AuroraState.idl`  
**Available task:** LowerBodyWBCLower, PdStand (in stance stage)

Publishes the current estimated pose of the robot's base relative to a pre-defined initial state. This includes changes in height, pitch, and yaw, as well as a stability level indicator.

```idl
struct RobotStandPoseState {     // RobotStand state 
    std_msgs::msg::Header header;
    double delta_z;             // z axis height
    double delta_pitch;         // pitch value 
    double delta_yaw;           // yaw value
    uint8 stable_level;         
};
```

### Public Subscriber

#### Whole Body FSM State Change Command

**Topic Name:** `whole_body_fsm_state_change_cmd`  
**Type Name:** `fourier_msgs::msg::WholeBodyFsmStateChangeCmd`  
**Locate in:** `AuroraCmd.idl`  
**Available task:** all tasks

Receives commands to change the whole body FSM state of Aurora to a specified target state. This allows external controllers or user interfaces to trigger state transitions such as switching between default, joint stand, pd stand, user-defined states, security, and command modes.

```idl
struct WholeBodyFsmStateChangeCmd {
    std_msgs::msg::Header header;
    uint8 desired_state;     // 0 defalut, 1 joint stand, 2 pd stand, 3~8 user a~f, 9 security, 10 usercmd, 11 upperbody usercmd
};
```

#### Upper Body FSM State Change Command

**Topic Name:** `upper_body_fsm_state_change_cmd`  
**Type Name:** `fourier_msgs::msg::UpperBodyFsmStateChangeCmd`  
**Locate in:** `AuroraCmd.idl`  
**Available task:** UpperBodyStateManager

Receives commands to change the upper body FSM state of Aurora to a specified target state. This enables external systems to control the upper body independently, such as switching between default, lower-upper linkage, or remote control modes.

```idl
struct UpperBodyFsmStateChangeCmd {
    std_msgs::msg::Header header;
    uint8 desired_state;     // 0 default, 1 lower-upper linkage, 2 remote ctrl
};
```

#### Velocity Source Command

**Topic Name:** `velocity_source_cmd`  
**Type Name:** `fourier_msgs::msg::VelocitySourceCmd`  
**Locate in:** `AuroraCmd.idl`  
**Available task:** all tasks

Receives commands to set the source of velocity commands for the robot, allowing dynamic switching between joystick, handheld, or navigation sources. For DDS control, please set velocity source command to 2 (navigation).

```idl
struct VelocitySourceCmd {
    std_msgs::msg::Header header;
    uint8 desired_source; // 0 joystick, 1 handheld, 2 navigation
};
```

#### Velocity Command

**Topic Name:** `velocity_cmd`  
**Type Name:** `fourier_msgs::msg::VelocityCmd`  
**Locate in:** `AuroraCmd.idl`  
**Available task:** LowerBodyCpgRL, LowerBodyWBCLower

Receives velocity commands for locomotion tasks, specifying the desired linear velocities (vx, vy) and yaw rate.

```idl
struct VelocityCmd {
    std_msgs::msg::Header header;
    double vx;
    double vy;
    double yaw;
};
```

#### Robot Control Group Command

**Topic Name:** `robot_control_group_cmd`  
**Type Name:** `fourier_msgs::msg::RobotControlGroupCmd`  
**Locate in:** `MotionControlCmd.idl`  
**Available task:** UserCmd, UpperBodyUserCmd, UpperBodyTeleTask, PdStand (in stance stage)

Receives direct joint commands for the robot, organized by control groups. For each group, it specifies the desired motion state, control type (joint or motor), motor mode (position or effort), and the target position, velocity, and effort values.

```idl
struct ControlGroupCmd {
    string group_name; // left_leg, right_leg, waist, head, left_manipulator, right_manipulator, left_hand, right_hand
    uint8 group_motion_state;  // 1 for move, 2 for stop, if stop ignore the rest fields
    uint8 control_type; // 0 for control joint, 1 for control motor
    uint8 motor_mode; // 0 for control position, 1 for control effort
    sequence<double> position; // for position control, this item is essential, for effort control, it will be ignore .
    sequence<double> velocity; // for position control, this is essential, for effort control this will be ignored .
    sequence<double> effort; // for position control, this is essential, for effort control this will be essential .
};

struct RobotControlGroupCmd {
    std_msgs::msg::Header header;
    sequence<ControlGroupCmd> group_cmd;
    sequence<std_msgs::msg::BaseDataType> ex_data; // extra data if needed
};
```

#### Robot Motor Config Group Command

**Topic Name:** `robot_motor_cfg_group_cmd`  
**Type Name:** `fourier_msgs::msg::RobotMotorCfgGroupCmd`  
**Locate in:** `MotorCfgCmd.idl`  
**Available task:** UserCmd, UpperBodyUserCmd

Receives configuration commands for actuator controllers, allowing the setting of proportional (P) and derivative (D) gains for each group.

```idl
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

#### Robot Stand Pose Command

**Topic Name:** `robot_stand_pose_cmd`  
**Type Name:** `fourier_msgs::msg::RobotStandPoseCmd`  
**Locate in:** `AuroraCmd.idl`  
**Available task:** LowerBodyWBCLower, PdStand (in stance stage)

Receives commands to adjust the robot's base height and orientation (pitch and yaw) relative to its initial pose.

```idl
struct RobotStandPoseCmd {     // RobotStand state 
    std_msgs::msg::Header header;
    double delta_z;             //  z axis height
    double delta_pitch;         // pitch value 
    double delta_yaw;           // yaw value
};
```
