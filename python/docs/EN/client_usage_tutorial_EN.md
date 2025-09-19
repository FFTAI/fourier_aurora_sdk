# Aurora Client Usage Tutorial

This tutorial will demonstrate how to create an Aurora client and exchange data with the Aurora through DDS communication.

## 1. Import Dependencies

First, import the necessary dependencies.

```python
from fourier_aurora_client import AuroraClient
```

## 2. Instantiate Aurora Client

```python
client = AuroraClient.get_instance(domain_id=160, participant_qos = 0,robot_name="gr2t2v2", serial_number=None)
```

Instantiate an Aurora client with the following parameters:

1. domain_id: DDS domain ID, default is 123.
2. participant_qos: QoS setting, default is 0.
3. robot_name: Robot name, default is "gr2t2v2".
4. serial_number: Robot serial number, default is None.

Note: The above parameters should match the DDS settings of the Aurora.

## 3. Data Preprocessing

```python
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
```

This tutorial aims to control the robot smoothly, so we need to interpolate the joint positions based on the initial and target joint positions, and convert the joint position commands into a dictionary format. The dictionary's key is the name of the control group, and the value is a list of the control group's commands.

## 4. Send Commands

```python
input("switch to PD stand task\nPlease desired state number: ")

client.set_fsm_state(2)
```

Send a command to switch to PD stand task.
```python
input("set robot joints to desired position\nPress Enter to continue...")
move_joints(client, INIT_POS, TARGET_POS, FREQUENCY, DURATION)

# Move back to initial position
input("set robot joints to home position\nPress Enter to continue...")
move_joints(client, TARGET_POS, INIT_POS, FREQUENCY, DURATION)
```

This code invokes the `move_joints` function to smoothly move the robot from its initial position to the target position and back. The function takes the initial position, target position, frequency, and duration as inputs, computes interpolated joint positions, and sends them via the client’s `set_joint_positions` function.

```python
    client.set_joint_positions(positions)
```

## 5. Close Client

```python
client.close()
```

After sending the commands, close the client.




