from fourier_aurora_client import AuroraClient
from fourier_aurora_client import MoveCommandManager
import time

# Initialize client
client = AuroraClient.get_instance(domain_id=136, robot_name='gr3')
move_command_manager = MoveCommandManager(robot_name='gr3')
occupied_groups = ["waist", "head", "left_manipulator", "right_manipulator"]
print_interval = 0.3
print("Initializing robot for joint control...")

# Step 1: Set FSM state to RL locomotion State
cmd = input("Press Enter to set FSM to RL locomotion (state 3)...")
client.set_fsm_state(3)
time.sleep(0.5)
client.set_upper_fsm_state(4)

# Step 2: Set Move Abs Joint Command
cmd = input("Press Enter to send move abs joint command...")
print("Sending move abs joint command...")

left_manipulator_target_pos =  [0.0, 0.0, 0.0, -1.2, 0.0, 0.0, 0.0]
right_manipulator_target_pos = [0.0, 0.0, 0.0, -1.2, 0.0, 0.0, 0.0]

move_command_manager.joint_move_command(
    move_type=0,
    group_name="left_manipulator",
    group_pos=left_manipulator_target_pos,
    expect_vel=300
)
move_command_manager.joint_move_command(
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

print(f"left_manipulator_position: {client.get_group_state('left_manipulator', 'position')}")
print(f"right_manipulator_position: {client.get_group_state('right_manipulator', 'position')}")
print(f"left_manipulator_pose: {client.get_cartesian_state('left_manipulator', 'pose')}")
print(f"right_manipulator_pose: {client.get_cartesian_state('right_manipulator', 'pose')}")

#Step 3: Set Move Joint Command
cmd = input("Press Enter to send move joint command...")
print("Sending move joint command...")
left_arm_pre_pose =  [0.3,  0.25, 0.2, 0.0, -0.7071, 0.0, 0.7071]
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

print(f"left_manipulator_pose: {client.get_cartesian_state('left_manipulator', 'pose')}")
print(f"right_manipulator_pose: {client.get_cartesian_state('right_manipulator', 'pose')}")

#Step 4: Set Move Joint Command
cmd = input("Press Enter to send move line command...")
print("Sending move line command...")
left_arm_pre_pose =  [0.3,  0.25, 0.3, 0.0, -0.7071, 0.0, 0.7071]
right_arm_pre_pose = [0.3, -0.25, 0.3, 0.0, -0.7071, 0.0, 0.7071]

move_command_manager.cartesian_move_command(
    move_type=2,
    group_name="left_manipulator",
    group_pos=left_arm_pre_pose,
    expect_vel=300
)
move_command_manager.cartesian_move_command(
    move_type=2,
    group_name="right_manipulator",
    group_pos=right_arm_pre_pose,
    expect_vel=300
)

move_command = move_command_manager.get_move_command()
client.set_move_command(move_command)
time.sleep(0.2)
client.wait_groups_motion_complete(occupied_groups, print_interval=print_interval)
time.sleep(0.5)

print(f"left_manipulator_pose: {client.get_cartesian_state('left_manipulator', 'pose')}")
print(f"right_manipulator_pose: {client.get_cartesian_state('right_manipulator', 'pose')}")

client.close()





