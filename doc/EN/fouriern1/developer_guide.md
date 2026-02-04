# Developer Guide

This guide is intended for developers who want to use Fourier Aurora SDK for their own applications on Fourier robots. It provides information on how you can use the SDK based on your project's needs.

Aurora runs a FSM state machine and each state corresponds to certain tasks(controllers). You can switch between these states using the joystick or by sending commands through the DDS interface. Every task has its own inputs and outputs. You can use **joystick** or **DDS** to interact with the tasks.

## State and Task Overview

Aurora provide various controllers natively, each controller is run by a task. The controllers cannot be called directly. Instead, they are wrapped in seperate states, which make sures the controllers won't overlap.

State name          | Joystick mapping | DDS mapping | Link
--------------------|------------------|-------------|-------------
Default             | LB+RB            | 0           | [Default State](controller_reference/default_state.md)
Security Protection | LT+RT            | 9           | [Security Protection State](controller_reference/security_protection_state.md)
Joint Stand         | LB+A             | 1           | [Joint Stand State](controller_reference/joint_stand_state.md)
PdStand             | LB+A             | 2           | [PdStand State](controller_reference/pd_stand_state.md)
RL Locomotion       | RB+A             | 3           | [RL Locomotion State](controller_reference/rl_locomotion_state.md)
UserCmd             | No               | 10          | [UserCmd State](controller_reference/user_cmd_state.md)
Upper UserCmd       | No               | 11          | [Upper UserCmd State](controller_reference/upper_user_cmd_state.md)

The state specification for each state is formatted as the same structure. Here's explanation on terms in state specification.

- **State Name**: Name of the state.
- **Task Name**: Name of the task, usually refers to the controller.
- **Joystick mapping**: The combination key to switch to that state using joystick.
- **DDS mapping**: The dds mapping value used to switch to that state using dds message or client.
- **Frequency**: The default frequency for the controllers in that state.
- **Avaliable for hanging**: Some controller (mostly reinforcement learning based controllers) might behave dangerous when their feets are off the ground. The section indicates whether the controllers in that state is safe for hanging.
- **Avaliable for standing**: This section indicates if the controllers in that state is able to stand on its own.
- **Auto Protection Switch**: Some controller comes with auto protection switch mechanism. When they finds themselves beyond the controllable range, they will automatically switch to security protection state. This section indicates whether controllers in that state comes with the auto protection switch mechanism.

## State Switch Interface

Each controller state in Aurora is mapped by a *DDS mapping value*, which can be found in the table above. Developers can use fourier aurora client to switch between these states and acquire current running state.

```python
client.set_fsm_state(2)     # switch to pdstand state

state = client.get_fsm_state()   # get dds mapping value for current state
```
