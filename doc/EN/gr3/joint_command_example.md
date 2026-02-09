# Joint Command Example

This document explains how to send joint-level commands to control the robot using the AuroraClient API. Aurora provides direct joint position, velocity, and torque control through control groups, as well as motor configuration adjustment capabilities. All commands must be sent through properly defined control groups to ensure coordinated motion.

## Control Unit

Aurora does not support direct control on actuators; all joint controls are aligned with the URDF model. For parallel mechanisms, the calculation is done within Aurora, and only joint-level state and command will be available. To achieve actuator-level control, please refer to *fourier_actuator_sdk*.

**Control group** is a group of connected controllable joints, and is an important concept in Aurora. For GR-3, the control groups are: `left_leg`, `right_leg`, `waist`, `head`, `left_manipulator`, `right_manipulator`. The minimum control unit in Aurora is a control group, meaning the developer must send the command for a complete control group each time.

**Important Notes:**

- Commands must include all joints in a control group
- Joint order must match the control group definition
- Commands sent to individual joints will be rejected
- Real-time control requires proper FSM state configuration

For detailed specifications on joints and control groups, please refer to [robot_specs](robot_specs.md)

## Joint Control

### set_group_cmd

The `set_group_cmd()` function sends position, velocity, and torque commands to one or multiple control groups. Position command is mandatory for each command, whereas velocity and torque command are default as 0. Each controllers in Aurora allows different sets of control groups to receive external command, as listed in the table below. Controller not listed cannot receive any group command.

controller    | left_leg | right_leg | head | waist | left_manipulator | right_manipulator
--------------|----------|-----------|------|-------|------------------|-------------
[PdStand State](controller_reference/pd_stand_state.md) | ❌ | ❌ | ✅ | ✅ | ✅ | ✅
[RL Locomotion State](controller_reference/rl_locomotion_state.md) | ❌ | ❌ | ✅ | ✅ | ✅ | ✅
[UserCmd State](controller_reference/user_cmd_state.md) | ✅ | ✅ | ✅ | ✅ | ✅ | ✅
[Upper UserCmd State](controller_reference/upper_user_cmd_state.md) | ❌ | ❌ | ✅ | ✅ | ✅ | ✅

Aurora runs PD control mode by default. Its torque output can be computed with:

$$
\tau = K_{p}(q_{cmd}-q) + K_{d}(\dot{q}_{cmd}-\dot{q}) + \tau_{cmd}
$$

For detail on actuator's control loop, please refer to *fourier_actuator_sdk*.

**Example - Direct Position Control:**

```python
left_manipulator_pos = [0.0, 0.0, 0.0, -0.2, 0.0, 0.0, 0.0]

# Control left manipulator - move to specific joint positions
position_cmd = {
    'left_manipulator': left_manipulator_pos
}

client.set_group_cmd(position_cmd=position_cmd)
print("Left manipulator position command sent")
```

**Example - Multi-Group Interpolate Control:**

```python
# Control multiple groups simultaneously with an interpolate motion
target_pos = {
    "left_manipulator_target" = [0.0, 0.0, 0.0, -1.0, 0.0, 0.0, 0.0],
    "right_manipulator_target" = [0.0, 0.0, 0.0, -1.0, 0.0, 0.0, 0.0],
    "waist_target" = [0.5, 0.0, 0.0]
}

init_pos = {
    "left_manipulator_target" = client.get_group_state("left_manipulator", key="position"),
    "right_manipulator_target" = client.get_group_state("right_manipulator", key="position"),
    "waist_target" = client.get_group_state("waist", key="position"),
}

total_step = 200
dt = 0.01
for step in range(total_step):
    pos_cmd = {}
    for control_group in target_pos.keys():
        group_pos_cmd = [init + (target - init) * step / total_step for init, target in zip(init_pos[control_group], target_pos[control_group])]
        pos_cmd[control_group] = group_pos_cmd

    client.set_group_cmd(position_cmd=pos_cmd)
    time.sleep(dt)
print("Multiple control groups commanded to target position")
```

**Safety Considerations:**

- Always verify joint limits before sending commands
- Apply interpolation to avoid sudden change in position
- Monitor robot state during motion

## Motor Configuration

### set_motor_cfg_pd

The `set_motor_cfg_pd()` function configures the PD (Proportional-Derivative) control gains for motors in specified control groups. Proper gain tuning is essential for stable and accurate motion control.

**Default Configuration:**
Aurora uses pre-tuned default gains for each control group. Each controller comes with a pretuned motor configuration. Some controller allows external motor config setup in order for developer usage. The avaliability for setting motor config in each controller is listed in the table below.

controller    | left_leg | right_leg | head | waist | left_manipulator | right_manipulator
--------------|----------|-----------|------|-------|------------------|-------------
[PdStand State](controller_reference/pd_stand_state.md) | ❌ | ❌ | ❌ | ❌ | ❌ | ❌
[RL Locomotion State](controller_reference/rl_locomotion_state.md) | ❌ | ❌ | ❌ | ❌ | ❌ | ❌
[UserCmd State](controller_reference/user_cmd_state.md) | ✅ | ✅ | ✅ | ✅ | ✅ | ✅
[Upper UserCmd State](controller_reference/upper_user_cmd_state.md) | ❌ | ❌ | ✅ | ✅ | ✅ | ✅

**Example - Setting Custom PD Gains:**

```python
# Configure left manipulator with moderate gains for compliant control
# Values are per-joint in the control group
left_manipulator_kp = [400, 200, 200, 200, 50, 50, 50]
left_manipulator_kd = [20, 10, 10, 10, 2.5, 2.5, 2.5]

kp_config = {'left_manipulator': left_manipulator_kp}
kd_config = {'left_manipulator': left_manipulator_kd}

client.set_motor_cfg_pd(kp_config=kp_config, kd_config=kd_config)
print("Left manipulator PD gains configured for compliant control")
time.sleep(1.0)

# Read back the configuration
actual_kp = client.get_group_motor_cfg('left_manipulator', 'pd_kp')
actual_kd = client.get_group_motor_cfg('left_manipulator', 'pd_kd')

print(f"Configured Kp: {kp_config['left_manipulator']}")
print(f"Actual Kp: {actual_kp}")
print(f"Configured Kd: {kd_config['left_manipulator']}")
print(f"Actual Kd: {actual_kd}")
```

## Complete Joint Control Example

Here's a complete example demonstrating motor configuration and joint control:

```python
from fourier_aurora_client import AuroraClient
import time

# Initialize client
client = AuroraClient.get_instance(domain_id=123, robot_name='gr3')

print("Initializing robot for joint control...")

# Step 1: Set FSM state to User Command State
cmd = input("Press Enter to set FSM to User Command State (state 10)...")
client.set_fsm_state(10)
time.sleep(0.5)

# Step 2: Configure motor gains
cmd = input("Press Enter to configure motor PD gains...")
print("Configuring motor PD gains...")
kp_config = {
    'left_manipulator': [400, 200, 200, 200, 50, 50, 50],
    'right_manipulator': [400, 200, 200, 200, 50, 50, 50],
    "waist": [200, 300, 200], 
    "head": [100, 100],
}
kd_config = {
    'left_manipulator': [20, 10, 10, 10, 2.5, 2.5, 2.5],
    'right_manipulator': [20, 10, 10, 10, 2.5, 2.5, 2.5],
    "waist": [10, 15, 10],
    "head": [10, 10],
}
client.set_motor_cfg_pd(kp_config=kp_config, kd_config=kd_config)
time.sleep(1.0)

# Read back the configuration
actual_kp = client.get_group_motor_cfg('left_manipulator', 'pd_kp')
actual_kd = client.get_group_motor_cfg('left_manipulator', 'pd_kd')

print(f"Configured Kp: {kp_config['left_manipulator']}")
print(f"Actual Kp: {actual_kp}")
print(f"Configured Kd: {kd_config['left_manipulator']}")
print(f"Actual Kd: {actual_kd}")

# Step 3: Get current positions
cmd = input("Press Enter to  move to target joint positions...")
print("Reading current positions...")
current_pos = {
    'left_manipulator': client.get_group_state('left_manipulator', key='position'),
    'right_manipulator': client.get_group_state('right_manipulator', key='position'),
    'waist': client.get_group_state('waist', key='position')
}
print(f"  Left manipulator: {[f'{p:.3f}' for p in current_pos['left_manipulator']]}")
print(f"  Right manipulator: {[f'{p:.3f}' for p in current_pos['right_manipulator']]}")
print(f"  Waist: {[f'{p:.3f}' for p in current_pos['waist']]}")

target_pos = {
    'left_manipulator': [0.0, 0.0, 0.0, -1.0, 0.0, 0.0, 0.0],
    'right_manipulator': [0.0, 0.0, 0.0, -1.0, 0.0, 0.0, 0.0],
    'waist': [0.5, 0.0, 0.0]
}

print("\nMoving to target position...")
total_steps = 200
dt = 0.01

for step in range(total_steps + 1):
    pos_cmd = {}
    for group_name in target_pos.keys():
        # Linear interpolation from current to target
        group_pos_cmd = [
            curr + (targ - curr) * step / total_steps 
            for curr, targ in zip(current_pos[group_name], target_pos[group_name])
        ]
        pos_cmd[group_name] = group_pos_cmd
    
    client.set_group_cmd(position_cmd=pos_cmd)
    time.sleep(dt)

print("Target position reached")
time.sleep(1.0)

# Step 4: Return to zero position with interpolation
cmd = input("Press Enter to return to zero position...")
print("\nReturning to zero position...")
zero_pos = {
    'left_manipulator': [0.0] * 7,
    'right_manipulator': [0.0] * 7,
    'waist': [0.0] * 3
}

# Get current position (should be at target)
start_pos = {
    'left_manipulator': client.get_group_state('left_manipulator', key='position'),
    'right_manipulator': client.get_group_state('right_manipulator', key='position'),
    'waist': client.get_group_state('waist', key='position')
}

for step in range(total_steps + 1):
    pos_cmd = {}
    for group_name in zero_pos.keys():
        # Linear interpolation from current to zero
        group_pos_cmd = [
            start + (zero - start) * step / total_steps 
            for start, zero in zip(start_pos[group_name], zero_pos[group_name])
        ]
        pos_cmd[group_name] = group_pos_cmd
    
    client.set_group_cmd(position_cmd=pos_cmd)
    time.sleep(dt)

print("Zero position reached")
time.sleep(0.5)

print("\nFinal robot state:")
final_left = client.get_group_state('left_manipulator', 'position')
final_right = client.get_group_state('right_manipulator', 'position')
final_waist = client.get_group_state('waist', 'position')
print(f"  Left manipulator: {[f'{p:.3f}' for p in final_left]}")
print(f"  Right manipulator: {[f'{p:.3f}' for p in final_right]}")
print(f"  Waist: {[f'{p:.3f}' for p in final_waist]}")

print("\nJoint control demonstration complete")
client.close()
```

This example demonstrates the complete workflow for joint control: configuring the FSM state, setting motor gains, reading current positions, moving to target positions with smooth interpolation, returning to zero position, and monitoring the final state.



