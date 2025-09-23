import time 
from fourier_aurora_client import AuroraClient

if __name__ == "__main__":

    # Initialize client
    client = AuroraClient.get_instance(domain_id=123, robot_name="PPWAVEV2", serial_number=None)
    time.sleep(1)
    
    # Use set_fsm_state() to switch FSM state. For state mapping please refer to the robot controller reference doc.
    cmd = input("Press Enter to switch to PdStand...")
    client.set_fsm_state(2)
    time.sleep(1.0)

    # Use set_stand_pose() to set stand pose.
    cmd = input("Press Enter to crouch...")
    client.set_stand_pose(-0.2, 0.0, 0.0)
    time.sleep(2.0)

    cmd = input("Press Enter to stand up...")
    client.set_stand_pose(0.0, 0.0, 0.0)
    time.sleep(2.0)

    # Use set_velocity_source() to set velocity command. For client velocity control, velocity source should be set to 2.
    cmd = input("Press Enter to enter walk task...")
    client.set_fsm_state(3)
    time.sleep(1.0)
    client.set_velocity_source(2)

    # Use set_vel() to set velocity command.
    cmd = input("Press Enter to walk...")
    client.set_velocity(0.2, 0.0, 0.0)
    time.sleep(1.0)

    # Use set_upper_fsm_state to change upper body state. For state mapping please refer to the robot controller reference doc.
    cmd = input("Press Enter to swing arms...")
    client.set_upper_fsm_state(1)
    time.sleep(1.0)

    cmd = input("Press Enter to stop walking...")
    client.set_velocity(0.0, 0.0, 0.0)
    time.sleep(1.0)

    client.close()
    print("User command test completed successfully.")

