# Upper UserCmd State Reference

Upon switching to **Upper UserCmd State**, the user can send external joint position commands for upper body joints, and actuator config setup commands such as pd parameters. The robot will execute these commands and update its states accordingly. At the same time, lower body actuators will be set to zero torque mode.

## State specification

State name       | Task name              | Joystick mapping | DDS mapping | Frequency
-----------------|------------------------|------------------|-------------|-------------
Upper UserCmd    | UpperBodyUserCmdTask   | No               | 10          | 400Hz

Avaliable for hanging | Avaliable for standing | Auto Protection Switch
----------------------|------------------------|----------------
Yes                   | No                     | No

## Joystick Control

No joystick control is avaliable for this state.

## Client Control

Velocity control | Stand pose control | Joint control | Joint parameter control
-----------------|--------------------|---------------|-------------------
No               | No                 | Whole Body    | Whole Body

### Enter UserCmd State

After initailize *AuroraCore*, use aurora client's `set_fsm_state` function to enter usercmd state.

```python
client = AuroraClient.get_instance(domain_id=123, robot_name="fouriern1")   # initialize aurora client
time.sleep(1)

client.set_fsm_state(11)     # change to upper usercmd state
```

### Joint Control

Joint control is avaliable via `set_group_cmd` function. Since the position command take effects immediately, it is suggested to use interpolation in upper body joint command to avoid sharp command change.

**Avaliable Control Groups:**  `Waist`, `Left_Manipulator`, `Right_Manipulator`

```python
left_manipulator_init_pose = client.get_group_state("left_manipulator", key="position")
left_manipulator_target_pose = [-1.2, 0, 0, 1.2, 0]
total_steps = 200

for i in range(total_steps):
    # interpolate from init to target pose
    left_manipulator_pose = [i + (t - i) * i / total_steps for i, t in zip(left_manipulator_init_pose, left_manipulator_target_pose)]
    client.set_group_cmd({"left_manipulator": left_manipulator_pose})
    time.sleep(0.01)
```

### Joint Parameter Control

Joint control is avaliable via `set_motor_cfg_pd` function. Currently, Aurora only supports pd control mode on all joints.

**Avaliable Control Groups:** `Waist`, `Left_Manipulator`, `Right_Manipulator`

```python
kp_config = {
    "waist": [90.0],
    "left_manipulator": [90.0, 45.0, 45.0, 45.0, 45.0],
    "right_manipulator": [90.0, 45.0, 45.0, 45.0, 45.0]
}
kd_config = {
    "waist": [8.0],
    "left_manipulator": [ 8.0, 2.5, 2.5, 2.5, 2.5],
    "right_manipulator": [8.0, 2.5, 2.5, 2.5, 2.5]
}

client.set_motor_cfg_pd(kp_config, kd_config)
```

For joint specifications, please refer to [robot_specs](../robot_specs_EN.md)
