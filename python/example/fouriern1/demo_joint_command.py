from fourier_aurora_client import AuroraClient
import time

# Initialize client
client = AuroraClient.get_instance(domain_id=123, robot_name='fouriern1')

print("Initializing robot for joint control...")

# Step 1: Set FSM state to User Command State
cmd = input("Press Enter to set FSM to User Command State (state 10)...")
client.set_fsm_state(10)
time.sleep(0.5)

# Step 2: Configure motor gains
cmd = input("Press Enter to configure motor PD gains...")
print("Configuring motor PD gains...")
kp_config = {
    'left_manipulator': [50.0] * 5,
    'right_manipulator': [50.0] * 5,
    'waist': [40.0]
}
kd_config = {
    'left_manipulator': [5.0] * 5,
    'right_manipulator': [5.0] * 5,
    'waist': [4.0]
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
    'left_manipulator': [0.0, 0.0, 0.0, 0.5, 0.0],
    'right_manipulator': [0.0, 0.0, 0.0, 0.5, 0.0],
    'waist': [0.5]
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
    'left_manipulator': [0.0] * 5,
    'right_manipulator': [0.0] * 5,
    'waist': [0.0]
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