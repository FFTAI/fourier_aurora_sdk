# Motion Command Example

This document explains how to send high-level motion commands to control the robot using the AuroraClient API. Motion commands include stand pose adjustment and velocity control, and each command is only available in specific controller states.

## Stand Pose Control

The `set_stand_pose()` function adjusts the robot standing pose in the PD stand controller. For `PdStand` state, stand pose adjustment is only available in the stance stage. The `get_stand_pose()` function reads the current stand pose (z, pitch, yaw) so you can verify the adjustment.

**Avaliable tasks for stand pose adjustment:** [PdStand State](controller_reference/pd_stand_state.md)

**Example - Adjust Stand Pose:**

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

## Velocity Control

### set_velocity_source

Before sending velocity commands, switch the velocity source so only one device controls the robot. Use `set_velocity_source()` where `0` refers to joystick control and `2` refers to client control.

### set_velocity

The `set_velocity()` function provides velocity control on x, y direction and yaw rotation in the RL locomotion controller. The duration option sets how long the velocity command is applied, and the robot will gradually slow down after the duration ends.

**Avaliable tasks for velocity command:** [RL Locomotion State](controller_reference/rl_locomotion_state.md)

**Example - Velocity Commands:**

```python
client.set_fsm_state(3)         # switch to rl locomotion state that allows velocity control
client.set_velocity_source(2)   # set velocity source to client control
time.sleep(0.5)

client.set_velocity(0.3, 0.0, 0.0, 5.0)  # make robot move forward at 0.3 m/s for 5 seconds
time.sleep(5.0)

client.set_velocity(0.0, 0.3, 0.0, 5.0)  # make robot move left at 0.3 m/s for 5 seconds
time.sleep(5.0)

client.set_velocity(0.0, 0.0, 0.5, 5.0)  # make robot turn left at 0.5 rad/s for 5 seconds
time.sleep(5.0)

client.set_velocity(0.0, 0.0, 0.0, 1.0)  # make robot stop
```

## Safety Considerations

- Always switch to the correct controller state before sending commands
- Set velocity source to avoid command conflicts with joystick control
- Use small increments when adjusting stand pose to avoid sudden motion
- Monitor robot state during motion commands

## Complete Motion Command Example

Here's a complete example demonstrating standpose and velocity control:

```python
from fourier_aurora_client import AuroraClient
import time

# Initialize client
client = AuroraClient.get_instance(domain_id=123)

print("Initializing motion control demo...")

# Step 1: Set FSM state to User Command State
cmd = input("Press Enter to set FSM to PdStand State (state 2)...")
client.set_fsm_state(2)
time.sleep(1.0)

# Step 2: PD stand pose adjustment
cmd = input("Press enter to start crouching and stand up...")
print("Adjusting stand pose to 0.10 meter lower to default pose")
client.set_stand_pose(-0.10, 0.0, 0.0)
time.sleep(2.0)
print(f"Current Stand pose: {client.get_stand_pose()}")

print("Adjusting stand pose back to default pose")
client.set_stand_pose(0.0, 0.0, 0.0)
time.sleep(2.0)

# Step 3: Set FSM state to Rl Locomotion State
cmd = input("Press Enter to set FSM to Rl Locomotion State (state 3)...")
client.set_fsm_state(3)
client.set_velocity_source(2)
time.sleep(0.5)

# Step 3: Set Velocity Command
cmd = input("Press Enter to set velocity command...")
print("Sending velocity commands...")
print("Moving forward at 0.2 m/s")
client.set_velocity(0.2, 0.0, 0.0, 5.0)
time.sleep(3.0)

print("Moving left at 0.2 m/s")
client.set_velocity(0.0, 0.2, 0.0, 5.0)
time.sleep(3.0)

print("Rotating anti-clockwise at 0.4 rad/s")
client.set_velocity(0.0, 0.0, 0.4, 5.0)
time.sleep(3.0)

print("Stopping")
client.set_velocity(0.0, 0.0, 0.0, 1.0)
time.sleep(1.0)

print("Motion command demonstration complete")
client.close()
```

This example demonstrates the usage on standpose and velocity control: switch to pdstand state then send standpose adjustment command; switch to rl locomotion state then send velocity command.
