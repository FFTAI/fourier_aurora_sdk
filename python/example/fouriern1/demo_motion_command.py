from fourier_aurora_client import AuroraClient
import time

# Initialize client
client = AuroraClient.get_instance(domain_id=123, robot_name='fouriern1')

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
time.sleep(5.0)

print("Moving left at 0.2 m/s")
client.set_velocity(0.0, 0.2, 0.0, 5.0)
time.sleep(5.0)

print("Rotating anti-clockwise at 0.4 rad/s")
client.set_velocity(0.0, 0.0, 0.4, 5.0)
time.sleep(5.0)

print("Stopping")
client.set_velocity(0.0, 0.0, 0.0, 1.0)
time.sleep(1.0)

print("Motion command demonstration complete")
client.close()