import time 
from fourier_aurora_client import AuroraClient

if __name__ == "__main__":

    # Initialize client
    client = AuroraClient.get_instance(domain_id=123, robot_name="fouriern1")
    time.sleep(1)
    
    # Use set_fsm_state() to switch FSM state. For state mapping please refer to the robot controller reference doc.
    cmd = input("Press Enter to switch to PdStand...")
    client.set_fsm_state(2)
    time.sleep(1.0)

    # Use set_velocity_source() to set velocity command. For client velocity control, velocity source should be set to 2.
    cmd = input("Press Enter to enter walk task...")
    client.set_fsm_state(3)
    time.sleep(1.0)
    client.set_velocity_source(2)

    # Use set_vel() to set velocity command.
    cmd = input("Press Enter to walk...")
    client.set_velocity(0.2, 0.0, 0.0, 3.0)
    time.sleep(1.0)

    client.close()
    print("client usage demo completed successfully.")