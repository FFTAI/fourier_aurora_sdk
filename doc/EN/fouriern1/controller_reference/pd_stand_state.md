# PdStand State Reference

**PdStand State** runs a pdstand controller that make the robot stand on a flat surface for further control. There are two stage in pdstand controller, *hanging stage* and *stance stage*. The stage change is determined by inner stable level estimator, where the user can acquire via client.

If stable level goes below 100 (for sim, below 10), the controller would switch to *hanging stage*. In *hanging stage*, all joints will slowly move back to default position, and no stand pose or joint control is avaliable.

If stable level goes above 100 (for sim, above 10), the controller would switch to *stance stage*. In *stance stage*, stand pose adjustment and upper body joint control are avaliable.

## State specification

State name | Task name     | Joystick mapping | DDS mapping | Frequency
-----------|---------------|------------------|-------------|-------------
PdStand    | PdStandTask   | LB+A             | 2           | 400Hz

Avaliable for hanging | Avaliable for standing | Auto Protection Switch
----------------------|------------------------|----------------
Yes                   | Yes                    | No

## Joystick Control

### Enter PdStand State

After initailize *AuroraCore*, press bumper `LB` and button `A` at the same time to enter pdstand state.

### Stand Pose Control

After the robot enter stance stage, use direction keys on the joystick to control robot's stand pose.

- press `up` or `down` direction key to control robot's height.
- press `left` or `right` direction key to control robot's pitch angle.
- Move the right rocker `left` or `right` to control robot's yaw angle.

### Reset button

PdStand State provides a shortcut key to reset robot's state. Long press button `A` and the robot will  

1. reset stand pose to default pose.
2. Gradually move arms to default position.

## Client Control

Velocity control | Stand pose control | Joint control | Joint parameter control
-----------------|--------------------|---------------|-------------------
No               | Yes                | Upper Body    | No

### Enter PdStand State

After initailize *AuroraCore*, use aurora client's `set_fsm_state` function to enter pdstand state.

```python
client = AuroraClient.get_instance(domain_id=123, robot_name="fouriern1")   # initialize aurora client
time.sleep(1)

client.set_fsm_state(2)     # change to pdstand state
```

### Stand Pose Control

Once the robot enter stance stage, stand pose control is avaliable via `set_stand_pose` function.

**Stand Pose Range:** `delta_z`: [0.01, -0.15], `delta_pitch`: [-0.2, 0.5], `delta_yaw`: [-0.3, 0.3]

```python
client.set_stand_pose(-0.1, 0.0, 0.0)   # crouch 0.1 meter
time.sleep(2.0)

client.set_stand_pose(0.0, 0.2, 0.0)    # lean forward 0.2 radian
time.sleep(2.0)

client.set_stand_pose(0.0, 0.0, 0.2)    # turn left 0.2 radian
time.sleep(2.0)

client.set_stand_pose(0.0, 0.0, 0.0)    # back to default pose
time.sleep(2.0)
```

### Joint Control

Once the robot enter stance stage, joint control is avaliable via `set_group_cmd` function. Since the position command take effects immediately, it is suggested to use interpolation in upper body joint command to avoid sharp command change.

**Avaliable Control Groups:** `Waist`, `Left_Manipulator`, `Right_Manipulator`

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

For joint specifications, please refer to [robot_specs](../robot_specs_EN.md)
