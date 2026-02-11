# Robot Status Example

This document explains how to acquire various robot status information using the AuroraClient API. The robot provides real-time state data including base motion data, contact information, joint states, and Cartesian states for different control groups. All robot status listed in this document is avalible throughout the runtime. All status update runs at 500Hz except motor configuration.

## Base Data

### get_base_data

The `get_base_data` function retrieves the robot's base motion data in both base coordinate and world coordinate reference frames. This includes orientation (as quaternions or Euler angles), velocities, accelerations, and positions.

**Coordinate system definition:**  
**base coordinate system:** The robot coordinate system is established at the robot's base center. The x-axis points towards the front direction of the base,  the y-axis points towards the left direction, and z-axis points towards upstraight direction. The x, y, and z axes obey the right-hand rule distribution.  
**world coordinate system:** The world coordinate system is established at the ground projection point of the robot's base center when Aurora is initialized. The orientation of x, y, and z axes is aligned with the world coordinate system.

**Available Keys:**

- `'quat_xyzw'` - Quaternion in x, y, z, w format
- `'quat_wxyz'` - Quaternion in w, x, y, z format
- `'rpy'` - Roll, pitch, yaw in radians
- `'omega_W'` - Angular velocity in world frame
- `'acc_W'` - Linear acceleration in world frame
- `'omega_B'` - Angular velocity in base frame
- `'acc_B'` - Linear acceleration in base frame
- `'vel_W'` - Linear velocity in world frame
- `'pos_W'` - Position in world frame
- `'vel_B'` - Linear velocity in base frame

**Example:**

```python
# Get base orientation as quaternion (x, y, z, w)
quat = client.get_base_data('quat_xyzw')
print(f"Base orientation (quat): {quat}")

# Get base orientation as roll, pitch, yaw
rpy = client.get_base_data('rpy')
print(f"Base orientation (RPY): roll={rpy[0]:.3f}, pitch={rpy[1]:.3f}, yaw={rpy[2]:.3f}")

# Get linear velocity in world frame
vel_world = client.get_base_data('vel_W')
print(f"Base velocity (world frame): {vel_world}")

# Get position in world frame
pos_world = client.get_base_data('pos_W')
print(f"Base position (world frame): {pos_world}")
```

## Contact Data

### get_contact_data

Aurora estimates contact force information using a disturbance observer based on generalized momentum, and decompose those disturbances into individual foot contact forces and probabilities. Developer can use `get_contact_data` function to retrieve contact force and probability.

**Example:**

```python
# Get contact forces
contact_forces = client.get_contact_data('contact_fz')
print(f"Contact forces: {contact_forces}")

# Get contact probabilities (useful for determining if feet are on ground)
contact_probs = client.get_contact_data('contact_prob')
print(f"Contact probabilities: {contact_probs}")

# Check if left and right feet are in contact
if len(contact_probs) >= 2:
    left_foot_contact = contact_probs[0] > 0.5
    right_foot_contact = contact_probs[1] > 0.5
    print(f"Left foot contact: {left_foot_contact}, Right foot contact: {right_foot_contact}")
```

## Joint State

### get_group_state

The `get_group_state()` function retrieves joint-level state information (positions, velocities, or efforts) for a specific control group.

**Example:**

```python
# Get left manipulator control group positions
left_manipulator_pos = client.get_group_state('left_manipulator', 'position')
print(f"Left manipulator joint positions: {left_manipulator_pos}")

# Get right manipulator control group velocities
right_manipulator_vel = client.get_group_state('right_manipulator', 'velocity')
print(f"Right manipulator joint velocities: {right_manipulator_vel}")

# Get head control group efforts
head_effort = client.get_group_state('head', 'effort')
print(f"Head joint efforts: {head_effort}")

# Monitor all joint states for a control group
print("\nComplete left manipulator state:")
print(f"  Positions: {client.get_group_state('left_manipulator', 'position')}")
print(f"  Velocities: {client.get_group_state('left_manipulator', 'velocity')}")
print(f"  Efforts: {client.get_group_state('left_manipulator', 'effort')}")
```

## Cartesian State

### get_cartesian_state

The `get_cartesian_state()` function retrieves Cartesian space information (pose, twist, or wrench) for the end-effector of a specific control group.

**Example:**

```python
# Get left manipulator end-effector pose
left_manipulator_pose = client.get_cartesian_state('left_manipulator', 'pose')
print(f"Left manipulator end-effector pose: {left_manipulator_pose}")
if len(left_manipulator_pose) >= 7:
    print(f"  Position: x={left_manipulator_pose[0]:.3f}, y={left_manipulator_pose[1]:.3f}, z={left_manipulator_pose[2]:.3f}")
    print(f"  Orientation (quat): qx={left_manipulator_pose[3]:.3f}, qy={left_manipulator_pose[4]:.3f}, qz={left_manipulator_pose[5]:.3f}, qw={left_manipulator_pose[6]:.3f}")

# Get right manipulator end-effector twist (velocity)
right_manipulator_twist = client.get_cartesian_state('right_manipulator', 'twist')
print(f"\nRight manipulator end-effector twist: {right_manipulator_twist}")
if len(right_manipulator_twist) >= 6:
    print(f"  Linear velocity: vx={right_manipulator_twist[0]:.3f}, vy={right_manipulator_twist[1]:.3f}, vz={right_manipulator_twist[2]:.3f}")
    print(f"  Angular velocity: wx={right_manipulator_twist[3]:.3f}, wy={right_manipulator_twist[4]:.3f}, wz={right_manipulator_twist[5]:.3f}")

# Get left manipulator end-effector wrench (force/torque)
left_manipulator_wrench = client.get_cartesian_state('left_manipulator', 'wrench')
print(f"\nLeft manipulator end-effector wrench: {left_manipulator_wrench}")
if len(left_manipulator_wrench) >= 6:
    print(f"  Force: fx={left_manipulator_wrench[0]:.3f}, fy={left_manipulator_wrench[1]:.3f}, fz={left_manipulator_wrench[2]:.3f}")
    print(f"  Torque: tx={left_manipulator_wrench[3]:.3f}, ty={left_manipulator_wrench[4]:.3f}, tz={left_manipulator_wrench[5]:.3f}")
```

## Motor Configuration

### get_group_motor_cfg

The `get_group_motor_cfg()` function retrieves the motor configuration parameters for a specific control group. This includes PID gains for different control modes. Motor configuration data updates at a rate of 1 hz.

Note: Currently Aurora only provides PD control modes in opensource version. Therefore, only `pd_kp` and `pd_kd` is valid for this function.

**Example:**

```python
# Get PD control mode gains for left manipulator
left_manipulator_kp = client.get_group_motor_cfg('left_manipulator', 'pd_kp')
left_manipulator_kd = client.get_group_motor_cfg('left_manipulator', 'pd_kd')
print(f"Left manipulator PD gains:")
print(f"  Kp: {left_manipulator_kp}")
print(f"  Kd: {left_manipulator_kd}")
```

## Complete Status Monitoring Example

Here's a complete example that monitors various robot states in a loop:

```python
from fourier_aurora_client import AuroraClient
import time

# Initialize client
client = AuroraClient.get_instance(domain_id=123, robot_name='gr2')

print("Starting robot status monitoring...")
print("Press Ctrl+C to stop\n")

try:
    while True:
        # Base status
        rpy = client.get_base_data('rpy')
        vel_world = client.get_base_data('vel_W')
        
        # Contact status
        contact_probs = client.get_contact_data('contact_prob')
        
        # Left manipulator status
        left_manipulator_pos = client.get_group_state('left_manipulator', 'position')
        left_manipulator_pose = client.get_cartesian_state('left_manipulator', 'pose')
        
        print(f"[{time.strftime('%H:%M:%S')}] Robot Status:")
        print(f"  Base RPY: [{rpy[0]:.3f}, {rpy[1]:.3f}, {rpy[2]:.3f}]")
        print(f"  Base Vel: [{vel_world[0]:.3f}, {vel_world[1]:.3f}, {vel_world[2]:.3f}]")
        print(f"  Contact: {['Yes' if p > 0.5 else 'No' for p in contact_probs]}")
        print(f"  Left manipulator Joints: {len(left_manipulator_pos)} DOF")
        print(f"  Left manipulator EE Pos: [{left_manipulator_pose[0]:.3f}, {left_manipulator_pose[1]:.3f}, {left_manipulator_pose[2]:.3f}]")
        print("-" * 60)
        
        time.sleep(0.5)  # Update every 0.5 seconds
        
except KeyboardInterrupt:
    print("\nStopping monitoring...")
    client.close()
```

This monitoring script provides a real-time view of the robot's base motion, contact states, and manipulator positions, which is useful for debugging and understanding robot behavior during operation.
