# RL Locomotion State Reference

**RL locomotion State** allows the robot to move with lower body while switch between arm swing and joint control in upper body. The lower body will execute a reinforcement learning based controller which allows velocity control in three directions.

On the other side, upper body is managed by a state manager task, which allows the user to switch between different upper body controllers. Avaliable controllers are:

- Default: no controller, mapped at upper body state 0.
- UpperBodyActTask: arm swing task, mapped at upper body state 1.
- UpperBodyTeleTask: joint control task, mapped at upper body state 2.

## State specification

State name | Task name                                      | Joystick mapping | DDS mapping | Frequency
-----------|------------------------------------------------|------------------|-------------|-------------
RL Locomotion  | LowerBodyCpgTask / UpperBodyStateManagerTask   | RB+A             | 3           | 50Hz / 500Hz

Avaliable for hanging | Avaliable for standing | Auto Protection Switch
----------------------|------------------------|----------------
No                    | Yes                    | No

## Joystick Control

### Enter RL Locomotion State

After initailize *AuroraCore*, press bumper `RB` and button `A` at the same time to enter RL locomotion state.

### Velocity Control

Use the left and right rocker to apply velocity control on the robot.

- left rocker vertical axis: move forward and backward
- left rocker horizontal axis: move left and right
- right rocker horizontal axis: turn left and right

### Arm Swing Switch

The upper body state manager provides a shortcut key to turn on arm swing task. Click button `B` to turn on arm swing, and then click `X` to turn off arm swing.

## Client Control

Velocity control | Stand pose control | Joint control | Joint parameter control
-----------------|--------------------|---------------|-------------------
Yes              | No                 | Upper Body    | No

### Enter RL Locomotion State

After initailize *AuroraCore*, use aurora client's `set_fsm_state` function to enter RL locomotion state.

```python
client = AuroraClient.get_instance(domain_id=123, robot_name="fouriern1")   # initialize aurora client
time.sleep(1)

client.set_fsm_state(3)     # change to RL locomotion state
```

### Velocity Control

Before applying velocity control through client, it is necessary to first switch velocity source in Aurora. Velocity source makes sure only one source can send velocity command at a time, ensure safety. Velocity source can be set with `set_velocity_source` function, where 0 refers to joystick control, and 2 refers to client control.

Once the velocity source is set to 2, velocity control is avaliable via `set_velocity` function.

**Velocity Range:** `vx`: [-0.5, 0.5], `vy`L [-0.5, 0.5], `vyaw`: [-1.0, 1.0]

```python
client.set_velocity_source(2)   # set velocity source to client control
time.sleep(0.5)

client.set_velocity(0.3, 0.0, 0.0, 5.0)  # make robot move forward at 0.5 m/s for 5 seconds
time.sleep(5.0)

client.set_velocity(0.0, 0.3, 0.0, 5.0)  # make robot move left at 0.3 m/s for 5 seconds
time.sleep(5.0)

client.set_velocity(0.0, 0.0, 0.5, 5.0)  # make robot turn left at 1.0 rad/s for 5 seconds
time.sleep(5.0)

client.set_velocity(0.0, 0.0, 0.0, 1.0)  # make robot stop 
```

### Arm Swing

To turn on arm swing task, change upper body state to 1.

```python
client.set_upper_fsm_state(1)   # turn on arm swing
```

### Joint Control

To apply joint control in RL locomotion state, it is necessaey to change upper body state to 2. 

Once upper state is changed to 2, joint control is avaliable via `set_group_cmd` function. Since the position command take effects immediately, it is suggested to use interpolation in upper body joint command to avoid sharp command change.

**Avaliable Control Groups:** `Waist`, `Left_Manipulator`, `Right_Manipulator`

```python
client.set_upper_fsm_state(2)   # turn on joint control

left_manipulator_init_pose = client.get_group_state("left_manipulator", key="position")
left_manipulator_target_pose = [-1.2, 0, 0, 1.2, 0]
total_steps = 200

for i in range(total_steps):
    # interpolate from init to target pose
    left_manipulator_pose = [i + (t - i) * i / total_steps for i, t in zip(left_manipulator_init_pose, left_manipulator_target_pose)]
    client.set_group_cmd({"left_manipulator": left_manipulator_pose})
    time.sleep(0.01)
```

For joint specifications, please refer to [robot_specs](../robot_specs_EN.md)
