# Motion Command Usage

The main functions of the motion command interface are robot complete machine control. Different controller will provide different motion command, including stand pose control and velocity control. Here's an demoinstraction on using these motion command on our robot.

## Stand Pose Command

The pdstand controller provides an option to adjust the standing pose of the robot. Developer can use `set_stand_pose` function in fourier aurora client to adjust its standing pose. Note that for pdstand state, stand pose adjustment is only avalibale in stance stage.

For developer that want to get current stand pose status, it is avaliable through `get_stand_pose` function.

**Avaliable tasks for stand pose adjustment:** [PdStand State](controller_reference/pd_stand_state.md)

**Example:**

```python
client.set_fsm_state(2)     # switch to pdstand state that allows stand pose adjustment
time.sleep(1.0)

client.set_stand_pose(-0.1, 0.0, 0.0)   # crouch 0.1 meter
time.sleep(2.0)
print(client.get_stand_pose())          # print current stand pose

client.set_stand_pose(0.0, 0.2, 0.0)    # lean forward 0.2 radian
time.sleep(2.0)
print(client.get_stand_pose())          # print current stand pose

client.set_stand_pose(0.0, 0.0, 0.2)    # turn left 0.2 radian
time.sleep(2.0)
print(client.get_stand_pose())          # print current stand pose

client.set_stand_pose(0.0, 0.0, 0.0)    # back to default pose
time.sleep(2.0)
print(client.get_stand_pose())          # print current stand pose
```

## Velocity Command

For the rl locomotion controller, the velocity command can be set through `set_velocity` function in fourier aurora client. The `set_velocity` functions provides velocity control on x, y direction and yaw rotation. The duration option sets the duration that the velocity command would execute, after that robot will gradually slow down.

Before applying velocity control through client, it is necessary to first switch velocity source in Aurora. Velocity source makes sure only one source can send velocity command at a time, ensure safety. Velocity source can be set with `set_velocity_source` function, where 0 refers to joystick control, and 2 refers to client control.

**Avaliable tasks for stand pose adjustment:** [RL Locomotion State](controller_reference/rl_locomotion_state.md)

```python
client.set_fsm_state(3)         # switch to rl locomotion state that allows velocity control
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
