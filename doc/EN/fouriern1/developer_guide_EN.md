# Developer Guide

This guide is intended for developers who want to use Fourier Aurora SDK for their own applications on Fourier robots. It provides information on how you can use the SDK based on your project's needs.

Aurora runs a FSM state machine and each state corresponds to certain tasks(controllers). You can switch between these states using the joystick or by sending commands through the DDS interface. Every task has its own inputs and outputs. You can use **joystick** or **DDS** to interact with the tasks.

## State and Task Overview

Aurora provide various controllers natively, each controller is run by a task. The controllers cannot be called directly. Instead, they are wrapped in seperate states, which make sures the controllers won't overlap.

State name          | Joystick mapping | DDS mapping | Link
--------------------|------------------|-------------|-------------
Default             | LB+RB            | 0           | [Default State](state_specification/default_state_EN.md)
Security Protection | LT+RT            | 9           | [Security Protection State](state_specification/security_protection_state_EN.md)
Joint Stand         | LB+A             | 1           | [Joint Stand State](state_specification/joint_stand_state_EN.md)
PdStand             | LB+A             | 2           | [PdStand State](state_specification/pd_stand_state_EN.md)
RL Locomotion       | RB+A             | 3           | [RL Locomotion State](state_specification/rl_locomotion_state_EN.md)
UserCmd             | No               | 10          | [UserCmd State](state_specification/user_cmd_state_EN.md)
Upper UserCmd       | No               | 11          | [Upper UserCmd State](state_specification/upper_user_cmd_state_EN.md)

The state specification for each state is formatted as the same structure. Here's explanation on terms in state specification.

- **State Name**: Name of the state.
- **Task Name**: Name of the task, usually refers to the controller.
- **Joystick mapping**: The combination key to switch to that state using joystick.
- **DDS mapping**: The dds mapping value used to switch to that state using dds message or client.
- **Frequency**: The default frequency for the controllers in that state.
- **Avaliable for hanging**: Some controller (mostly reinforcement learning based controllers) might behave dangerous when their feets are off the ground. The section indicates whether the controllers in that state is safe for hanging.
- **Avaliable for standing**: This section indicates if the controllers in that state is able to stand on its own.
- **Auto Protection Switch**: Some controller comes with auto protection switch mechanism. When they finds themselves beyond the controllable range, they will automatically switch to security protection state. This section indicates whether controllers in that state comes with the auto protection switch mechanism.

## Aurora State Interface

Each controller state in Aurora is mapped by a *DDS mapping value*, which can be found in the table above. Developers can user aurora python client to switch between these states.

```python
client.set_fsm_state(state: int)
```

## Control Unit

Aurora does not support direct control on actuators, all joint controls are aligned with the urdf model. For parallel mechanism, the calculation is done within Aurora, and only joint level state and command will be avaliable. To achive actuator level control, please refer to *fourier_actuator_sdk*.

**control group** is a group of connected controllable joints, and is an important concept in Aurora. For fourier n1, the control groups are: `Left_Leg`, `Right_Leg`, `Waist`, `Left_Manipulator`, `Right_Manipulator`. The minimum control unit in Aurora is control group, meaning the user have to send the command for a complete control group each time.

For detailed specification on joints and control groups, please refer to [robot_specs](robot_specs_EN.md)