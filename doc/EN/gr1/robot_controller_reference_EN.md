# GR-1 Robot Controller Reference

This is the reference for controllers provided in the fourier-aurora and expansion packages. Controllers are provided as **Tasks** in FSM States. Aurora runs a FSM state machine and each state corresponds to certain tasks(controllers). You can switch between these states using the joystick or by sending commands through the DDS interface. Every task has its own inputs and outputs.

Aurora provides a set of controllers that can be used to control the robot's motion. Some universal controllers are provided in the `fourier-aurora` package, while others are provided in the expansion packages according to the robot configuration. Use `fourier-aurora-gr1` for GR-1 robot.

For DDS reference, please refer to [DDS Interface Reference](../aurora_dds_reference_EN.md).

## Universal Controllers

State name | Task name | Joystick mapping | DDS mapping | Description
-----------|-----------|------------------|------------|------------
Default | DefaultTask | LB+RB | 0 | Default state, do nothing, used as the default state after starting the FSM.
SecurityProtection | SecurityProtectionTask | LT+RT | 9 | Safety state, stops all actuators for emergency situations. Will reactivate the actuators when exiting this state.
JointStand | JointStandTask | LB+A | 1 | All actuators move to zero position. Used to check if all actuator joints are working properly and zero calibrations are done.
PdStand | PdStandTask | LB+B | 2 | Robot stand on flat surface, supports external joint position commands for upper body joints(waist, arms and head) and stand pose adjustment including base height, pitch and yaw.
UserCmd | UserCmdTask | - | 10 | Excute external joint control and config commands.
UpperBodyUserCmd | UpperBodyUserCmdTask | - | 11 | Excute external joint control and config commands for upper body joints(waist, arms and head).

### PdStand Task Specification

This task is used to make the robot stand on a flat surface for further control. Upon switching to PdStand State, if the robot is in hanging state, it will first bend its knees for stance preparation. Then you can place the robot on a flat surface vertically(no external force in z axis, e.g. suspension cable). When the stable level reaches 100, the robot will enter stance state and the user can control the robot's joints using external joint position commands for upper body joints(waist, arms and head) and base height, pitch and yaw commands in delta format(relative to init pose).

INPUTS:

- External joint position commands for upper body joints(waist, arms and head).
  - Unit: radian
  - Range: according to urdf joint limits
  - Note: only take effect when robot is in stance state.
  - Reference: [Robot Control Group Command](../aurora_dds_reference_EN.md#robot-control-group-command)

- Base height, pitch and yaw commands in delta format(relative to init pose).
  - Unit: meter for height, radian for pitch and yaw
  - Range(gr2): delta_z [-0.2, 0.02], delta_pitch [-0.3, 0.45], delta_yaw [-0.5, 0.5]
  - Note: only take effect when robot is in stance state.
  - Reference: [Robot Stand Pose Command](../aurora_dds_reference_EN.md#robot-stand-pose-command)

OUTPUTS:

- Base height, pitch and yaw states in delta format(relative to init pose).
  - Unit: meter for height, radian for pitch and yaw
  - Range: delta_z [-0.2, 0.02], delta_pitch [-0.3, 0.45], delta_yaw [-0.5, 0.5]
  - Note: only take effect when robot is in stance state.
  - Reference: [Robot Stand Pose State](../aurora_dds_reference_EN.md#robot-stand-pose-state)

- Stable level
  - Unit: -
  - Range: [0,110] for real robot, [0,15] for sim
  - Note: when stable level is below 100, real robot will enter hanging state.
  - Reference: [Robot Stand Pose State](../aurora_dds_reference_EN.md#robot-stand-pose-state)

### UserCmd Task Specification

This task is used to execute external joint control and config commands. Upon switching to UserCmd State, the user can send external joint position commands for all joints, and external config commands such as pd parameters. The robot will execute these commands and update its states accordingly.

INPUTS:

- External joint position commands for all joints.
  - Rnit: radian for position referece, radian/s for velocity reference, Nm for torque forward feedback
  - Range: according to urdf joint limits
  - Reference: [Robot Control Group Command](../aurora_dds_reference_EN.md#robot-control-group-command)

- External config commands such as pd parameters.
  - Unit: -
  - Range: -
  - Reference: [Robot Motor Config Group Command](../aurora_dds_reference_EN.md#robot-motor-config-group-command)

### UpperBodyUserCmd Task Specification

This task is used to execute external joint control and config commands for upper body joints(waist, arms and head). Upon switching to UpperBodyUserCmd State, leg actuators will be disabled until exicting this state.

INPUTS:

- External joint position commands for upper body joints(waist, arms and head).
  - Unit: radian for position referece, radian/s for velocity reference, Nm for torque forward feedback
  - Range: according to urdf joint limits
  - Reference: [Robot Control Group Command](../aurora_dds_reference_EN.md#robot-control-group-command)

- External config commands such as pd parameters.
  - Unit: -
  - Range: -
  - Reference: [Robot Motor Config Group Command](../aurora_dds_reference_EN.md#robot-motor-config-group-command)

## GR-1 Controllers

State name | Task name | Joystick mapping | DDS mapping | Description
-----------|-----------|------------------|-------------|------------
UserController_A | LowerBodyCpgRLTask UpperBodyStateManagerTask | RB+A | 3 | Straight Leg RL policy walking, receives external velocity commands(vx, vy, vyaw), supports external joint control commands for upper body joints(waist, arms, head and hands)

### LowerBodyCpgRLTask Specification

This task is used to control the lower body joints(hips, knees, and ankles) using policy, enabling the robot to walk. It is recommand to swtich to pd stand state first to ensure the robot is standing on a flat surface. Upon switching to LowerBodyCpgRLTask, the robot will execute CPG policy to control the lower body joints.

INPUTS:

- Velocity commands (vx, vy, vyaw).
  - Unit: m/s for vx and vy, radian/s for vyaw
  - Range: vx [-0.4, 0.5], vy [-0.3, 0.3], vyaw [-0.8, 0.8]
  - Reference: [Velocity commands](../aurora_dds_reference_EN.md#velocity-command)

### UpperBodyStateManagerTask Specification

This task is used to manage the upper body joints(waist, arms and head). It is usually used in combination with a lower body controller. Upon switching to UpperBodyStateManagerTask State, the robot will execute a state machine to manage the upper body joints. The initial state is "default". User can switch to other states to make arms swing or receive external joint control commands for upper body joints(waist, arms and head). **Now it is recommended to use pd stand for standing upper body control for stability.**

INPUTS:

- Upper body state change commands.
  - Unit: -
  - Range: 0 default, 1 lower-upper linkage, 2 remote ctrl
  - Reference: [Upper Body FSM State Change Command](../aurora_dds_reference_EN.md#upper-body-fsm-state-change-command)

- External joint position commands for upper body joints(waist, arms and head).
  - Unit: radian for position referece, radian/s for velocity reference, Nm for torque forward feedback
  - Range: according to urdf joint limits
  - Note: only take effect when upper body state is "remote ctrl".
  - Reference: [Robot Control Group Command](../aurora_dds_reference_EN.md#robot-control-group-command)
  