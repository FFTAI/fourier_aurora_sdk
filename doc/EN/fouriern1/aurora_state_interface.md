# Aurora State Interface

Aurora maintains an inner finite state machine to manage different controllers. Developers can switch between these controllers with *aurora python client*.

## Wholebody fsm state

Wholebody fsm state is the overall state of Aurora. Currently avaliable wholbody fsm states are:

State name          | DDS mapping | Link
--------------------|-------------|-------------
Default             | 0           | [Default State](state_specification/default_state_EN.md)
Security Protection | 9           | [Security Protection State](state_specification/security_protection_state_EN.md)
Joint Stand         | 1           | [Joint Stand State](state_specification/joint_stand_state_EN.md)
PdStand             | 2           | [PdStand State](state_specification/pd_stand_state_EN.md)
CPG Walk            | 3           | [CPG Walk State](state_specification/cpg_walk_state_EN.md)
UserCmd             | 10          | [UserCmd State](state_specification/user_cmd_state_EN.md)
Upper UserCmd       | 11          | [Upper UserCmd State](state_specification/upper_user_cmd_state_EN.md)

### Python Interface Description

**Function Name**           | **set_fsm_state**
----------------------------|-----------------------------------------
Function Prototype          | set_fsm_state(state: int) -> None
Function Overview           | Set the robot's whole body fsm state.
Parameter                   | **state** (int): Desired fsm state of the robot.
Return Value                | None

**Function Name**           | **get_fsm_state**
----------------------------|-----------------------------------------
Function Prototype          | get_fsm_state() -> int
Function Overview           | Get the robot's whole body fsm state.
Parameter                   | None
Return Value                | (int): DDS mapping value of current fsm state.

### Upperbody fsm state

For **RL Locomotion State**, an upper body finite state machine is used to manage its upper body controller.

State name          | Upper DDS mapping
--------------------|-------------
Default             | 0
Arm Swing           | 1
Joint Control       | 2
Move Command        | 4

### Python Interface Description

**Function Name**           | **set_upper_fsm_state**
----------------------------|-----------------------------------------
Function Prototype          | set_upper_fsm_state(state: int) -> None
Function Overview           | Set the robot's upper body fsm state.
Parameter                   | **state** (int): Desired upper body fsm state of the robot.
Return Value                | None

**Function Name**           | **get_upper_fsm_state**
----------------------------|-----------------------------------------
Function Prototype          | get_upper_fsm_state() -> int
Function Overview           | Get the robot's upper body fsm state.
Parameter                   | None
Return Value                | (int): Upper DDS mapping value of current upper fsm state.
