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