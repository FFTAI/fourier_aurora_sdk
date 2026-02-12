# Manipulator Move Command Example

This document explains how to send move commands to control the robot's upper control group using the AuroraClient API and MoveCommandManager. Move commands provide high-level motion planning for both joint-level and cartesian-level movements with trajectory interpolation, velocity control, and motion completion tracking.

## Move Command Overview

Move commands are available in specific controller states and provide three types of motion:

- **MOVE_ABS_JOINT (type 0)**: Absolute joint position control with trajectory planning
- **MOVE_JOINT (type 1)**: Cartesian pose control with joint-space interpolation
- **MOVE_LINE (type 2)**: Cartesian linear motion with straight-line path planning

Unlike direct joint control, move commands automatically handle trajectory interpolation, velocity profiling, and motion completion detection, making them ideal for coordinated multi-group movements.

**Available controller states for move commands:**

- [RL Locomotion State](controller_reference/rl_locomotion_state.md) with Upper FSM State set to Move Command (state 4)

## MoveCommandManager

The `MoveCommandManager` class simplifies the creation and management of move commands. It handles the message structure and allows you to configure multiple control groups before sending the command.

**Initialize MoveCommandManager:**

```python
from fourier_aurora_client import MoveCommandManager

# Initialize with robot name
move_command_manager = MoveCommandManager(robot_name='gr3')
```

## Joint-Level Move Commands

### joint_move_command

The `joint_move_command()` function creates an absolute joint position move command with trajectory planning. This is useful for moving joints to specific positions with controlled velocity.

**Avaliable Control Groups:** `waist`, `head`, `left_manipulator`, `right_manipulator`

**Parameters:**

- `move_type` (int): Must be 0 for MOVE_ABS_JOINT
- `group_name` (str): Control group name (e.g., 'left_manipulator', 'right_manipulator')
- `group_pos` (list[float]): Target joint positions for the control group
- `group_vel` (list[float], optional): Target joint velocities (currently not used)
- `expect_vel` (int, optional): Expected velocity as thousandths of maximum speed (default: 1000)

**Example - Absolute Joint Position Control:**

```python
# Define target joint positions
left_manipulator_target_pos = [0.0, 0.0, 0.0, -1.2, 0.0, 0.0, 0.0]
right_manipulator_target_pos = [0.0, 0.0, 0.0, -1.2, 0.0, 0.0, 0.0]

# Configure move commands for both manipulators
move_command_manager.joint_move_command(
    move_type=0,
    group_name="left_manipulator",
    group_pos=left_manipulator_target_pos,
    expect_vel=300  # 30% of maximum velocity
)
move_command_manager.joint_move_command(
    move_type=0,
    group_name="right_manipulator",
    group_pos=right_manipulator_target_pos,
    expect_vel=300
)

# Get the configured move command and send it
move_command = move_command_manager.get_move_command()
client.set_move_command(move_command)
time.sleep(0.2)

# Wait for motion to complete
occupied_groups = ["left_manipulator", "right_manipulator"]
client.wait_groups_motion_complete(occupied_groups, print_interval=0.3)
```

## Cartesian-Level Move Commands

### cartesian_move_command

The `cartesian_move_command()` function creates cartesian-space move commands. It supports two motion types:

- **MOVE_JOINT (type 1)**: Moves end-effector to target pose using joint-space interpolation
- **MOVE_LINE (type 2)**: Moves end-effector to target pose following a straight line in cartesian space

The target pose is specified as [x, y, z, qx, qy, qz, qw], where (x, y, z) is position in meters and (qx, qy, qz, qw) is orientation as a quaternion. All Cartesian commands are expressed in the robot’s base coordinate frame, centered at the robot base.

**Avaliable Control Groups:** `left_manipulator`, `right_manipulator`

**Parameters:**

- `move_type` (int): 1 for MOVE_JOINT or 2 for MOVE_LINE
- `group_name` (str): Control group name
- `group_pos` (list[float]): Target cartesian pose [x, y, z, qx, qy, qz, qw]
- `group_vel` (list[float], optional): Target cartesian velocity (currently not used)
- `expect_vel` (int, optional): Expected velocity as thousandths of maximum speed (default: 1000)

**Example - Joint-Space Cartesian Motion:**

```python
# Define target poses (position + quaternion orientation)
left_arm_pose = [0.3, 0.25, 0.2, 0.0, -0.7071, 0.0, 0.7071]
right_arm_pose = [0.3, -0.25, 0.2, 0.0, -0.7071, 0.0, 0.7071]

# Move with joint-space interpolation
move_command_manager.cartesian_move_command(
    move_type=1,  # MOVE_JOINT
    group_name="left_manipulator",
    group_pos=left_arm_pose,
    expect_vel=300
)
move_command_manager.cartesian_move_command(
    move_type=2,  # MOVE_LINE
    group_name="right_manipulator",
    group_pos=right_arm_pose,
    expect_vel=300
)

move_command = move_command_manager.get_move_command()
client.set_move_command(move_command)
time.sleep(0.2)
client.wait_groups_motion_complete(occupied_groups, print_interval=0.3)

# Verify final poses
print(f"Left manipulator pose: {client.get_cartesian_state('left_manipulator', 'pose')}")
print(f"Right manipulator pose: {client.get_cartesian_state('right_manipulator', 'pose')}")
```

## Motion Completion Tracking

### wait_groups_motion_complete

The `wait_groups_motion_complete()` function blocks execution until all specified control groups have completed their motion. This is essential for sequential move commands.

```python
# Define all groups involved in the motion
occupied_groups = ["waist", "head", "left_manipulator", "right_manipulator"]

# Wait for all groups to complete motion
client.wait_groups_motion_complete(
    occupied_groups, 
    print_interval=0.3  # Print status every 0.3 seconds
)
```

Alternatively, use `get_group_motion_state()` to check individual groups:

```python
# Check if a specific group has completed motion
motion_state = client.get_group_motion_state('left_manipulator')
# Returns: 0 if motion complete, non-zero if still moving
```

## Safety Considerations

- **Controller State**: Always set the correct FSM state (RL Locomotion) and upper FSM state (Move Command) before sending move commands
- **Velocity Limits**: Use appropriate `expect_vel` values. Start with lower values (200-400) for testing
- **Motion Completion**: Always wait for motion to complete before sending the next command
- **Joint Limits**: Verify target positions are within joint limits
- **Collision Avoidance**: Ensure target poses do not cause self-collision or workspace violations
- **Coordinate Frames**: Cartesian poses are specified in the robot's base frame

## Complete Move Command Example

Here's a complete example demonstrating all three move command types:

```python
from fourier_aurora_client import AuroraClient
from fourier_aurora_client import MoveCommandManager
import time

# Initialize client and move command manager
client = AuroraClient.get_instance(domain_id=123, robot_name='gr3')
move_command_manager = MoveCommandManager(robot_name='gr3')
occupied_groups = ["waist", "head", "left_manipulator", "right_manipulator"]
print_interval = 0.3

print("Initializing robot for move command control...")

# Step 1: Set FSM states
cmd = input("Press Enter to set FSM to RL locomotion with Move Command state...")
client.set_fsm_state(3)  # RL Locomotion State
time.sleep(0.5)
client.set_upper_fsm_state(4)  # Move Command State
print("FSM states configured")

# Step 2: Move to initial position using absolute joint commands
cmd = input("Press Enter to send move abs joint command...")
print("Sending move abs joint command...")

left_manipulator_target_pos = [0.0, 0.0, 0.0, -1.2, 0.0, 0.0, 0.0]
right_manipulator_target_pos = [0.0, 0.0, 0.0, -1.2, 0.0, 0.0, 0.0]

move_command_manager.joint_move_command(
    move_type=0,
    group_name="left_manipulator",
    group_pos=left_manipulator_target_pos,
    expect_vel=300
)
move_command_manager.cartesian_move_command(
    move_type=0,
    group_name="right_manipulator",
    group_pos=right_manipulator_target_pos,
    expect_vel=300
)

move_command = move_command_manager.get_move_command()
client.set_move_command(move_command)
time.sleep(0.2)
client.wait_groups_motion_complete(occupied_groups, print_interval=print_interval)
time.sleep(0.5)

print(f"Left manipulator position: {client.get_group_state('left_manipulator', 'position')}")
print(f"Right manipulator position: {client.get_group_state('right_manipulator', 'position')}")
print(f"Left manipulator pose: {client.get_cartesian_state('left_manipulator', 'pose')}")
print(f"Right manipulator pose: {client.get_cartesian_state('right_manipulator', 'pose')}")

# Step 3: Move to target pose using joint-space interpolation
cmd = input("Press Enter to send move joint command (cartesian with joint interpolation)...")
print("Sending move joint command...")

left_arm_pre_pose = [0.3, 0.25, 0.2, 0.0, -0.7071, 0.0, 0.7071]
right_arm_pre_pose = [0.3, -0.25, 0.2, 0.0, -0.7071, 0.0, 0.7071]

move_command_manager.cartesian_move_command(
    move_type=1,
    group_name="left_manipulator",
    group_pos=left_arm_pre_pose,
    expect_vel=300
)
move_command_manager.cartesian_move_command(
    move_type=1,
    group_name="right_manipulator",
    group_pos=right_arm_pre_pose,
    expect_vel=300
)

move_command = move_command_manager.get_move_command()
client.set_move_command(move_command)
time.sleep(0.2)
client.wait_groups_motion_complete(occupied_groups, print_interval=print_interval)
time.sleep(0.5)

print(f"Left manipulator pose: {client.get_cartesian_state('left_manipulator', 'pose')}")
print(f"Right manipulator pose: {client.get_cartesian_state('right_manipulator', 'pose')}")

# Step 4: Move to final pose using linear cartesian motion
cmd = input("Press Enter to send move line command (cartesian linear motion)...")
print("Sending move line command...")

left_arm_final_pose = [0.3, 0.25, 0.3, 0.0, -0.7071, 0.0, 0.7071]
right_arm_final_pose = [0.3, -0.25, 0.3, 0.0, -0.7071, 0.0, 0.7071]

move_command_manager.cartesian_move_command(
    move_type=2,
    group_name="left_manipulator",
    group_pos=left_arm_final_pose,
    expect_vel=300
)
move_command_manager.cartesian_move_command(
    move_type=2,
    group_name="right_manipulator",
    group_pos=right_arm_final_pose,
    expect_vel=300
)

move_command = move_command_manager.get_move_command()
client.set_move_command(move_command)
time.sleep(0.2)
client.wait_groups_motion_complete(occupied_groups, print_interval=print_interval)
time.sleep(0.5)

print(f"Left manipulator pose: {client.get_cartesian_state('left_manipulator', 'pose')}")
print(f"Right manipulator pose: {client.get_cartesian_state('right_manipulator', 'pose')}")

print("\nMove command demonstration complete")
client.close()
```

This example demonstrates the complete workflow for move command control: setting FSM states, using absolute joint commands, cartesian joint-space interpolation, and linear cartesian motion with proper motion completion tracking.
