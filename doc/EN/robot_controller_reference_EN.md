# Robot Controller Reference

This is the reference for controllers provided in the fourier-aurora and expansion packages. Controllers are provided as **Tasks** in FSM States. Aurora runs a FSM state machine and each state corresponds to certain tasks(controllers). You can switch between these states using the joystick or by sending commands through the DDS interface. Every task has its own inputs and outputs.

Aurora provides a set of controllers that can be used to control the robot's motion. Some universal controllers are provided in the `fourier-aurora` package, while others are provided in the expansion packages according to the robot configuration, such as `fourier-aurora-gr2` for GR-2 robot.

For DDS reference, please refer to [DDS Interface Reference](aurora_dds_reference_EN.md).

## Universal Controllers

State name | Task name | Joystick mapping | DDS mapping | Description
-----------|-----------|------------------|------------|------------
Default | DefaultTask | LB+RB | 0 | Default state, do nothing, used as the default state after starting the FSM.
SecurityProtection | SecurityProtectionTask | LT+RT | 9 | Safety state, stops all actuators for emergency situations. Will reactivate the actuators when exiting this state.
JointStand | JointStandTask | LB+A | 1 | All actuators move to zero position. Used to check if all actuator joints are working properly and zero calibrations are done.
PdStand | PdStandTask | LB+B | 2 | Robot stand on flat surface, supports external joint position commands for upper body joints(waist, arms and head) and stand pose adjustment including base height, pitch and yaw. 
UserCmd | UserCmdTask | - | 10 | Excute external joint control and config commands.
UpperBodyUserCmd | UpperBodyUserCmdTask | - | 11 | Excute external joint control and config commands for upper body joints(waist, arms and head).

#### PdStand Task Specification

This task is used to make the robot stand on a flat surface for further control. Upon switching to PdStand State, if the robot is in hanging state, it will first bend its knees for stance preparation. Then you can place the robot on a flat surface vertically(no external force in z axis, e.g. suspension cable). When the stable level reaches 100, the robot will enter stance state and the user can control the robot's joints using external joint position commands for upper body joints(waist, arms and head) and base height, pitch and yaw commands in delta format(relative to init pose).

INPUTS:
- External joint position commands for upper body joints(waist, arms and head).
    - unit: radian
    - range: according to urdf joint limits
    - Note: only take effect when robot is in stance state.

- Base height, pitch and yaw commands in delta format(relative to init pose).
    - unit: meter for height, radian for pitch and yaw
    - range(gr2): delta_z [-0.2, 0.025], delta_pitch [-0.3, 0.45], delta_yaw [-0.5, 0.5]
    - Note: only take effect when robot is in stance state.

OUTPUTS:
- Base height, pitch and yaw states in delta format(relative to init pose).
    - unit: meter for height, radian for pitch and yaw
    - range: delta_z [-0.2, 0.025], delta_pitch [-0.3, 0.45], delta_yaw [-0.5, 0.5]
    - Note: only take effect when robot is in stance state.

- Stable level 
    - unit: -
    - range: [0,110] for real robot, [0,15] for sim
    - Note: when stable level is below 100, real robot will enter hanging state.

#### UserCmd Task Specification

This task is used to execute external joint control and config commands. Upon switching to UserCmd State, the user can send external joint position commands for all joints, and external config commands such as pd parameters. The robot will execute these commands and update its states accordingly.

INPUTS:
- External joint position commands for all joints.
    - unit: radian for position referece, radian/s for velocity reference, Nm for torque forward feedback
    - range: according to urdf joint limits

- External config commands such as pd parameters.
    - unit: -
    - range: -

#### UpperBodyUserCmd Task Specification

This task is used to execute external joint control and config commands for upper body joints(waist, arms and head). Upon switching to UpperBodyUserCmd State, leg actuators will be disabled until exicting this state. 

INPUTS:
- External joint position commands for upper body joints(waist, arms and head).
    - unit: radian for position referece, radian/s for velocity reference, Nm for torque forward feedback
    - range: according to urdf joint limits

- External config commands such as pd parameters.
    - unit: -
    - range: -


## GR-2 Controllers

State name | Task name | Joystick mapping | DDS mapping | Description
-----------|-----------|------------------|------------|------------
UserController_A | LowerBodyCpgTask UpperBodyStateManagerTask | LB+A | 3 | RL policy walking, receives external velocity commands(vx, vy, vyaw), supports external joint control commands for upper body joints(waist, arms and head)

#### LowerBodyCpgTask Specification

This task is used to control the lower body joints(hips, knees, and ankles) using policy, enabling the robot to walk. It is recommand to swtich to pd stand state first to ensure the robot is standing on a flat surface. Upon switching to LowerBodyCpgTask State, the robot will execute CPG policy to control the lower body joints. 

INPUTS:
- Velocity commands (vx, vy, vyaw).
    - unit: m/s for vx and vy, radian/s for vyaw
    - range: according to robot capability

#### UpperBodyStateManagerTask Specification

This task is used to manage the upper body joints(waist, arms and head). It is usually used in combination with a lower body controller. Upon switching to UpperBodyStateManagerTask State, the robot will execute a state machine to manage the upper body joints. The initial state is "default". User can switch to other states to make arms swing or receive external joint control commands for upper body joints(waist, arms and head). **Now it is recommended to use pd stand for standing upper body control for stability.**

INPUTS:
- Upper body state change commands.
    - unit: -
    - range: 0 defalut, 1 lower-upper linkage, 2 remote ctrl
- External joint position commands for upper body joints(waist, arms and head).
    - unit: radian for position referece, radian/s for velocity reference, Nm for torque forward feedback
    - range: according to urdf joint limits
    - Note: only take effect when upper body state is "remote ctrl".