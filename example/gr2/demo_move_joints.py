import time 
from fourier_aurora_client import AuroraClient

def interpolate_position(init_pos, target_pos, step, total_steps):
    """Linear interpolation between initial and target positions"""
    return [i + (t - i) * step / total_steps for i, t in zip(init_pos, target_pos)]

def move_joints(client, init_pos, target_pos, frequency, duration):
    """Move joints from initial to target position with linear interpolation"""
    total_steps = int(frequency * duration)
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
    client = AuroraClient.get_instance(domain_id=136, robot_name="gr2t2v2", serial_number=None)
    time.sleep(1)
    
    """
    The minimum control unit in AuroraClient is Control Group, meaning you have to set the command for a whole contorl group at once. 
    In this tutorial we will use a simple linear interpolation to move the joints from initial to target position.
    In concern of safety, we recommand you to use interpolator for any direct joint command.
    """

    # Here's and example on how to use AuroraClient to move the joints of a robot.
    cmd = input("Press Enter to start the joint position demo...") 
    print("switching to PdStand...")
    client.set_fsm_state(2)
    time.sleep(1.0)

    cmd = input("Press Enter to start left manipulator movement...") 
    left_manipulator_init_pose = client.get_group_state("left_manipulator")
    left_manipulator_target_pose = [-0.2, 0, 0, -1.2, 0, 0, 0]

    for i in range(200):
        left_manipulator_pose = interpolate_position(left_manipulator_init_pose, left_manipulator_target_pose, i, 200)
        client.set_joint_positions({"left_manipulator": left_manipulator_pose})
        time.sleep(0.01)
    
    cmd = input("Press Enter to start left hand movement...") 
    left_hand_init_pose = [0.2, 0.2, 0.2, 0.2, 1.2, 0.0]
    left_hand_target_pose = [1.7, 1.7, 1.7, 1.7, 0.0, 0.0]

    for i in range(100):
        left_hand_pose = interpolate_position(left_hand_init_pose, left_hand_target_pose, i, 100)
        client.set_joint_positions({"left_hand": left_hand_pose})
        time.sleep(0.01)
    
    # use the move joint function provided in this script to move multiple control groups at once
    cmd = input("Press Enter to start moving all control groups back to initial pose...")
    init_pos = {
        "left_leg": client.get_group_state("left_leg"),
        "right_leg": client.get_group_state("right_leg"),
        "waist": client.get_group_state("waist"),
        "head": client.get_group_state("head"),
        "left_manipulator": client.get_group_state("left_manipulator"),
        "right_manipulator": client.get_group_state("right_manipulator"),
        "left_hand": [1.7, 1.7, 1.7, 1.7, 0.0, 0.0],
        "right_hand": [1.7, 1.7, 1.7, 1.7, 0.0, 0.0]
    }
    target_pos = {
        "left_leg": [0, 0, 0, 0, 0, 0],
        "right_leg": [0, 0, 0, 0, 0, 0],
        "waist": [0],
        "head": [0, 0],
        "left_manipulator": [0, 0, 0, 0, 0, 0, 0],
        "right_manipulator": [0, 0, 0, 0, 0, 0, 0],
        "left_hand": [0.2, 0.2, 0.2, 0.2, 1.2, 0.0],
        "right_hand": [0.2, 0.2, 0.2, 0.2, 1.2, 0.0]
    }
    move_joints(client, init_pos, target_pos, frequency=100, duration=2.0)

    
    client.close()
    print("User command test completed successfully.")



