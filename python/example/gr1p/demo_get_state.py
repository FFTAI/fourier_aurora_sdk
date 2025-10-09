import time 
from fourier_aurora_client import AuroraClient

if __name__ == "__main__":

    # Initialize client
    client = AuroraClient.get_instance(domain_id=123, robot_name="gr1t2", serial_number=None)
    time.sleep(1)
    
    try:
        while True:
            print("\nCurrent States: ")
            # get aurora state
            fsm_state = client.get_fsm_state()
            upper_fsm_state = client.get_upper_fsm_state()
            velocity_source = client.get_velocity_source()
            print(f"fsm_state: {fsm_state}, upper_fsm_state: {upper_fsm_state}, velocity_source: {velocity_source}")

            # get robot base stand pose and velocity
            stand_pose = client.get_stand_pose()
            base_velocity = client.get_base_data("vel_B")
            base_angular_velocity = client.get_base_data("omega_B")
            print(f"stand_pose: {stand_pose} \
                \nbase_velocity: {base_velocity} \
                \nbase_angular_velocity: {base_angular_velocity}")

            # get joint group state and cartesian state
            left_leg_joint_state = client.get_group_state("left_leg")
            left_manipulator_cartesian_state = client.get_cartesian_state("left_manipulator")
            print(f"left_leg_joint_state: {left_leg_joint_state} \
                \nleft_manipulator_cartesian_state: {left_manipulator_cartesian_state}")
            
            time.sleep(1)
    except KeyboardInterrupt:
        pass

    finally:
        client.close()
        time.sleep(0.5)
        print("get state demo completed successfully.")

