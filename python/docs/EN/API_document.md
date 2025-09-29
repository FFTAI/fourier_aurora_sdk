# Fourier Aurora Client API Documentation

## Function List

- [get_instance](#get_instance)
- [close](#close)
- [get_fsm_state](#get_fsm_state)
- [get_upper_fsm_state](#get_upper_fsm_state)
- [get_velocity_source](#get_velocity_source)
- [get_stand_pose](#get_stand_pose)
- [get_group_state](#get_group_state)
- [get_cartesian_state](#get_cartesian_state)
- [get_base_data](#get_base_data)
- [get_contact_data](#get_contact_data)
- [set_fsm_state](#set_fsm_state)
- [set_upper_fsm_state](#set_upper_fsm_state)
- [set_velocity_source](#set_velocity_source)
- [set_velocity](#set_velocity)
- [set_stand_pose](#set_stand_pose)
- [set_joint_positions](#set_joint_positions)
- [set_motor_cfg](#set_motor_cfg)

## User Interface

### get_instance

```python
def get_instance(cls, domain_id: int =123, participant_qos = 0, robot_name: str = "gr2t2v2", serial_number = None) -> "AuroraClient":
```

Initializes an AuroraClient instance.

- Args
  - domain_id (int): The domain ID of the DDS domain. It should be matched with the domain ID of the Aurora server.
  - participant_qos(int): The Quality of Service (QoS) settings for the DDS DomainParticipant.
  - robot_name(str): The name of the robot.
  - serial_number: serial_number of the robot.

- Returns
  - AuroraClient: An instance of the AuroraClient class.

### close

```python
def close(self)
```

Close the AuroraClient instance.

## Getter Function

### get_fsm_state

```python
def get_fsm_state(self) -> str:
```

Get the current state of the robot.

- Returns
  - str: The name of the current whole body fsm state. Possible return values are:
    - 0 - Default state
    - 1 - Joint stand
    - 2 - PD stand
    - 3~8 - User controller A~F
    - 9 - Security protection
    - 10 - User command
    - 11 - Upper body user command

### get_upper_fsm_state

```python
def get_upper_fsm_state(self) -> str:
```

Returns the current upper body FSM state name.

- Returns
  - str: The name of the current upper body fsm state. Possible return values are:
    - 0 - Default state
    - 1 - Act state (Arm swing)
    - 2 - Remote state

### get_velocity_source

```python
def get_velocity_source(self) -> str:
```

Get the current velocity command source of the robot.

- Returns
  - str: The name of the current velocity command source. Possible return values are:
    - 0 - Joystick
    - 1 - Handheld device
    - 2 - Navigation

### get_stand_pose

```python
def get_stand_pose(self) -> list
```

Get the stand pose of the robot.

- Returns
  - list[float]: stand pose data in a list, formatted as [delta_z, delta_pitch, delta_yaw, stable_level]

### get_group_state

```python
def get_group_state(self, group_name: str, key: str = 'position') -> list[float]
```

Get the state of a specific robot control group

- Args
  - group_name (str): The name of the robot control group.
  - key (str): The key to retrieve from the group state. Valid values are:
    - 'position' - Joint positions
    - 'velocity' - Joint velocities
    - 'effort' - Joint efforts

- Returns
  - list[float]: The requested state data for the specified group.

### get_cartesian_state

```python
def get_cartesian_state(self, group_name: str, key: str = 'pose') -> list[float]
```

Get the cartesian state of a specific robot group.

- Args
  - group_name (str): The name of the robot group.
  - key (str): The key to retrieve from the cartesian state. Valid values are:
    - 'pose' - Group pose
    - 'twist' - Group twist
    - 'wrench' - Group wrench

- Returns
  - list[float]: The requested state cartesian for the specified group.

### get_base_data

```python
def get_base_data(self, key: str) -> list[float]
```

Get the base data of the robot.

- Args
  - key (str): The key to retrieve from the base data.
    - 'quat_xyzw' - quaternion in x, y, z, w format
    - 'quat_wxyz' - quaternion in w, x, y, z format
    - 'rpy' - roll, pitch, yaw in radians
    - 'omega_W' - angular velocity in world frame
    - 'acc_W' - linear acceleration in world frame
    - 'omega_B' - angular velocity in base frame
    - 'acc_B' - linear acceleration in base frame
    - 'vel_W' - linear velocity in world frame
    - 'pos_W' - position in world frame
    - 'vel_B' - linear velocity in base frame

- Returns
  - list[float]: The requested base data.

### get_contact_data

```python
def get_contact_data(self, key: str) -> list[float]
```

Get the contact data of the robot.

- Args
  - key (str): The key to retrieve from the contact data.
    - 'contact_fz' - contact force and torque
    - 'contact_prob' - contact probability

- Returns
  - list[float]: The requested contact data.

## Setter Function

### set_fsm_state

```python
def set_fsm_state(self, state: int)
```

Set the robot's whole body fsm state.

- Args
  - state (int): Desired state of the robot. Valid values are:
    - 0 - Default state
    - 1 - Joint stand
    - 2 - PD stand
    - 3~8 - User controller A~F
    - 9 - Security protection
    - 10 - User command
    - 11 - Upper body user command

### set_upper_fsm_state

```python
def set_upper_fsm_state(self, state: int)
```

Set the robot's upper body fsm state.

- Args
  - state (int): Desired upper state of the robot. Valid values are:
    - 0 - Default state
    - 1 - Act state (Arm swing)
    - 2 - Remote state

### set_velocity_source

```python
def set_velocity_source(self, source: int)
```

Set the source of velocity commands.

- Args
  - source (int): Desired velocity command source. Valid values are:
    - 0 - Joystick
    - 1 - Handheld device
    - 2 - Navigation

- Note
  - For client velocity control, set source to 2 (Navigation).

### set_velocity

```python
def set_velocity(self, vx: float, vy: float, yaw: float)
```

Set the robot's velocity command.

- Args
  - vx (float): Linear velocity in x direction (forward/backward) in m/s
  - vy (float): Linear velocity in y direction (left/right) in m/s
  - yaw (float): Angular velocity around z axis (rotation) in rad/s

- Note
  - The actual velocity achieved may be limited by the robot's physical capabilities.

### set_stand_pose

```python
def set_stand_pose(self, delta_z: float, delta_pitch: float, delta_yaw: float)
```

Set the robot's stand pose command.

- Args
  - delta_z (float): Desired change in base height (in meters).
                     Positive values raise the robot, negative values lower it.
  - delta_pitch (float): Desired change in pitch angle (in radians).
                         Positive values tilt the robot forward, negative backward.
  - delta_yaw (float): Desired change in yaw angle (in radians).
                       Positive values turn the robot left, negative right.

- Note
  - Stand pose adjustment is only available in PdStand state (2). The actual stand pose may be limited by the robot's physical capabilities.

### set_joint_positions

```python
def set_joint_positions(self, position_dict: Dict[str, list[float]], is_upper: bool = True)
```

Set the robot's joint positions.

- Args
  - position_dict (Dict[str, list[float]]): Dictionary with joint names as keys and joint positions as values.

### set_motor_cfg

```python
def set_motor_cfg(self, kp_config: Dict[str, list[float]], kd_config: Dict[str, list[float]])
```

Set the robot's motor configuration.

- Args
  - kp_config (Dict[str, list[float]]): Dictionary with group names as keys and proportional gain (kp) values as lists.
  - kd_config (Dict[str, list[float]]): Dictionary with group names as keys and derivative gain (kd) values as lists.

- Note
  - The keys in kp_config and kd_config should match the group names in the robot's motor configuration.
  - The values in kp_config and kd_config should be lists of floats representing the gains for each joint in the group.
