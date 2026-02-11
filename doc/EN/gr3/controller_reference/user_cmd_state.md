# UserCmd State Reference

Upon switching to **UserCmd State**, the user can send external joint position commands for all joints, and actuator config setup commands such as pd parameters. The robot will execute these commands and update its states accordingly.

## State specification

State name | Task name     | Joystick mapping | DDS mapping | Frequency
-----------|---------------|------------------|-------------|-------------
UserCmd    | UserCmdTask   | No               | 10          | 400Hz

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
client = AuroraClient.get_instance(domain_id=123, robot_name="gr3")   # initialize aurora client
time.sleep(1)

client.set_fsm_state(10)     # change to usercmd state
```

### Joint Control

Joint control is avaliable via `set_group_cmd` function. Since the position command take effects immediately, it is suggested to use interpolation in upper body joint command to avoid sharp command change.

**Avaliable Control Groups:** `left_leg`, `right_leg`, `waist`, `head`, `left_manipulator`, `right_manipulator`

```python
left_manipulator_init_pose = client.get_group_state("left_manipulator", key="position")
left_manipulator_target_pose = [0.0, 0.0, 0.0, -1.2, 0.0, 0.0, 0.0]
total_steps = 200

for i in range(total_steps):
    # interpolate from init to target pose
    left_manipulator_pose = [i + (t - i) * i / total_steps for i, t in zip(left_manipulator_init_pose, left_manipulator_target_pose)]
    client.set_group_cmd({"left_manipulator": left_manipulator_pose})
    time.sleep(0.01)
```

### Joint Parameter Control

Joint control is avaliable via `set_motor_cfg_pd` function. Currently, Aurora only supports pd control mode on all joints.

**Avaliable Control Groups:** `left_leg`, `right_leg`, `waist`, `head`, `left_manipulator`, `right_manipulator`

```python
kp_config = {
    "left_leg": [400, 200, 200, 400, 200, 26],
    "right_leg": [400, 200, 200, 400, 200, 26],
    "waist": [200, 300, 200], 
    "head": [100, 100],
    "left_manipulator": [400, 200, 200, 200, 50, 50, 50],
    "right_manipulator": [400, 200, 200, 200, 50, 50, 50],
}
kd_config = {
    "left_leg": [20, 20, 20, 20, 20, 2.6],
    "right_leg": [20, 20, 20, 20, 20, 2.6],
    "waist": [10, 15, 10],
    "head": [10, 10]
    "left_manipulator": [20, 10, 10, 10, 2.5, 2.5, 2.5],
    "right_manipulator": [20, 10, 10, 10, 2.5, 2.5, 2.5],
}

client.set_motor_cfg_pd(kp_config, kd_config)
```

For joint specifications, please refer to [robot_specs](../robot_specs.md)
