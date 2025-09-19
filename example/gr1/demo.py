import time 
from fourier_aurora_client import AuroraClient

def interpolate_position(init_pos, target_pos, step, total_steps):
    """Linear interpolation between initial and target positions"""
    return [i + (t - i) * step / total_steps for i, t in zip(init_pos, target_pos)]

def move_joints(client, init_pos, target_pos, frequency, duration):
    """Move joints from initial to target position with linear interpolation"""
    total_steps = frequency * duration
    for step in range(total_steps + 1):
        positions = {
            "left_leg": interpolate_position(init_pos["left_leg"], target_pos["left_leg"], step, total_steps),
            "right_leg": interpolate_position(init_pos["right_leg"], target_pos["right_leg"], step, total_steps),
            "waist": interpolate_position(init_pos["waist"], target_pos["waist"], step, total_steps),
            "head": interpolate_position(init_pos["head"], target_pos["head"], step, total_steps),
            "left_manipulator": interpolate_position(init_pos["left_manipulator"], target_pos["left_manipulator"], step, total_steps),
            "right_manipulator": interpolate_position(init_pos["right_manipulator"], target_pos["right_manipulator"], step, total_steps),
            "left_hand": interpolate_position(init_pos["left_hand"], target_pos["left_hand"], step, total_steps),
            "right_hand": interpolate_position(init_pos["right_hand"], target_pos["right_hand"], step, total_steps),
        }
        client.set_joint_positions(positions)
        time.sleep(1 / frequency)


if __name__ == "__main__":

    # Initialize client
    client = AuroraClient.get_instance(123, robot_name="gr1t2", serial_number=None)
    time.sleep(1)

    # Configuration
    FREQUENCY = 225
    DURATION = 3

    # Position definitions
    INIT_POS = {
        "left_leg": client.get_group_state("left_leg"),
        "right_leg": client.get_group_state("right_leg"),
        "waist": client.get_group_state("waist"),
        "head": client.get_group_state("head"),
        "left_manipulator": client.get_group_state("left_manipulator"),
        "right_manipulator": client.get_group_state("right_manipulator"),
        "left_hand": [0, 0, 0, 0, 0, 0],
        "right_hand": [0, 0, 0, 0, 0, 0],
    }

    TARGET_POS = {
        "left_leg": [0, 0, 0, 0, 0.7, 0],
        "right_leg": [0, 0, 0, 0, 0.7, 0],
        "waist": [0, 0, 0], 
        "head": [0, 0, 0],
        "left_manipulator": [0, 0, 0, -0.7, 0, 0, 0],
        "right_manipulator": [0, 0, 0, -0.7, 0, 0, 0],
        "left_hand": [1.5, 1.5, 1.5, 1.5, 0.5, 0.5],
        "right_hand": [1.5, 1.5, 1.5, 1.5, 0.5, 0.5],
    }


#----------------------------------------------move joint example----------------------------------------------------------------
    print("\nTesting move joint function..")

    # use pd stand fot upper joint control
    client.set_fsm_state(2)

    # use usercontrollerA fot upper joint control
    # client.set_fsm_state(3)
    # client.set_upper_fsm_state(2)

    input("set robot joints to desired position\nPress Enter to continue...")
    move_joints(client, INIT_POS, TARGET_POS, FREQUENCY, DURATION)

    # Move back to initial position
    input("set robot joints to home position\nPress Enter to continue...")
    move_joints(client, TARGET_POS, INIT_POS, FREQUENCY, DURATION)

#----------------------------------------------set velocity example----------------------------------------------------------------
    # print("\nTesting set velocity function..")
    # input("please make sure your aurora have install locomotion task in UserControllerA\nPress Enter to continue...")
    # client.set_fsm_state(3)     # set state to UserControllerA
    # client.set_upper_fsm_state(1)   # set upper state to Act State (Arm Swing)
    # client.set_velocity_source(2)   # set velocity source to Navigation

    # # move forward
    # input("send move forward command\nPress Enter to continue...")
    # client.set_velocity(1, 0, 0)
    # # move left
    # input("send move left command\nPress Enter to continue...")
    # client.set_velocity(0, 1, 0)

    # # rotate clockwise
    # input("send rotate clockwise command\nPress Enter to continue...")
    # client.set_velocity(0, 0, 1)
    # # stop
    # input("send stop command\nPress Enter to continue...")
    # client.set_velocity(0, 0, 0)
    # client.set_velocity_source(0)

#----------------------------------------------set stand pose example----------------------------------------------------------------
    # client.set_fsm_state(2)
    # client.set_velocity_source(2)
    
    # input("start sending pose cmd\nPress Enter to continue...")
    # client.set_stand_pose(-0.2, 0, 0)
    # input("start sending pose cmd\nPress Enter to continue...")
    # client.set_stand_pose(-0.2, 0.2, 0)
    # input("start sending pose cmd\nPress Enter to continue...")
    # client.set_stand_pose(-0.2, 0.2, 0.2)


    # input("stop sending pose cmd\nPress Enter to continue...")    
    # client.set_stand_pose(0, 0, 0)
    
    # input("start sending back pose cmd\nPress Enter to continue...")
    # client.set_stand_pose(1, 1, 1)
    # input("stop sending back pose cmd\nPress Enter to continue...")
    # client.set_stand_pose(0, 0, 0)

#----------------------------------------------subscriber example----------------------------------------------------------------
    # while True:
    #     try:
    #         print(f"state: {client.get_fsm_state()}")
    #         print(f"upper_state: {client.get_upper_fsm_state()}")
    #         print(f"velocity_command_source: {client.get_velocity_source()}")
    #         print(f"stand_pose: {client.get_stand_pose()}")
    #         print(f"left_leg: {client.get_group_state('left_leg')}")
    #         print(f"left_manipulator: {client.get_cartesian_state('left_manipulator')}")
    #         print(f"base: rpy: {client.get_base_data('rpy')}")
    #         print(f"true: vel_B: {client.get_true_data('vel_B')}")
    #         print(f"contact_prob: {client.get_contact_data('contact_prob')}")
            
    #     except KeyError as e:
    #         print(f"Error: {e}")
            
    #     time.sleep(0.1)
    

    client.close()
    print("User command test completed successfully.")


