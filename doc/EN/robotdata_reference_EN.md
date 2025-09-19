# RobotData Reference

This is the reference for the `RobotData` data struct shared in all tasks.

`RobotData` includes several sub data structs, covering different aspects of the robot's state and control. Here is a brief overview of each sub data struct, including its members and the functions that can be used to access and modify its contents.

## Struct: `RobotData`

| Member Type           | Member Name     | Description                             |
|------------------------|------------------|-----------------------------------------------------|
| `JoyStickData`         | `joyStick`       | Joystick control input data                        |
| `FsmStateData`         | `fsmState`       | Finite State Machine (FSM) state                   |
| `FourierHardwareData`  | `hardwareData`   | Data from the Fourier hardware layer               |
| `GeneralizedState`     | `generState`     | Generalized robot state                            |
| `KindynData`           | `kindynData`     | Kinematics and dynamics-related data               |
| `BaseData`             | `baseData`       | Robot base data (e.g., base position, velocity)    |
| `ContactData`          | `contactData`    | Contact information (e.g., foot contact)           |

### JoyStickData

| Member Type | Member Name | Description |
|-------------|-------------|-------------|
| `Eigen::Vector3d`    | `liner_cmd`         |  Velocity command (x, y, z) |

### FourierHardwareData

| Return Type                   | Function & Parameters                                                                 | Note                                                         |
|------------------------------|----------------------------------------------------------------------------------------|--------------------------------------------------------------|
| `grx_sot::robot::RobotState` | `getRobotStateFixed()`                                                                 | Get current state of the robot                               |
| `bool`                       | `setWholeBodyPosCmd(const Eigen::VectorXd& pos_cmd)`                                  | Set position command for the whole body joints               |
| `bool`                       | `setControlGroupPosCmd(std::string group_name, Eigen::VectorXd pos_cmd)`              | Set position command for a control group                     |
| `int`                        | `setWholeBodyPDModePD(const Eigen::VectorXd& kp, const Eigen::VectorXd& kd)`          | Set PD control gains (Kp, Kd) for whole body                 |
| `bool`                       | `setControlGroupPDModePD(std::string group_name, const Eigen::VectorXd& kp, kd)`      | Set PD gains for a specific control group                    |
| `void`                       | `getWholeBodyPDModePD(Eigen::VectorXd& kp, Eigen::VectorXd& kd)`                      | Get current whole body PD gains                              |
| `void`                       | `getControlGroupPDModePD(std::string group_name, Eigen::VectorXd& kp, kd)`            | Get PD gains of a specific control group                     |
| `std::vector<std::string>`   | `getHandNameList()`                                                                   | Get list of hand names                                       |
| `int`                        | `setHandPVC(std::string hand_name, std::vector<float> pos_cmd, std::vector<float> vel_cmd, std::vector<float> acc_cmd)`    | Set position, velocity and accerlation command for a specific hand   |
| `std::vector<std::vector<float>>`  | `getHandPVC(std::string hand_name)`                                             | Get current position, velocity and accerlation of a specified hand   |

- `RobotState`

| Member Type        | Member Name | Description                        |
|--------------------|-------------|------------------------------------|
| `Eigen::VectorXd`  | `q`         | Joints configuration               |
| `Eigen::VectorXd`  | `qd`        | Joints velocity                    |
| `Eigen::VectorXd`  | `qdd`       | Joints acceleration                |
| `Eigen::VectorXd`  | `qft`       | Joints force or torque             |

- Joint Sequence

| Index | Joint Name                    | Group Name |
|-------|-------------------------------|------------|
| 0     | left_hip_pitch_joint          | left_leg   |
| 1     | left_hip_roll_joint           | left_leg   |
| 2     | left_hip_yaw_joint            | left_leg   |
| 3     | left_knee_pitch_joint         | left_leg   |
| 4     | left_ankle_pitch_joint        | left_leg   |
| 5     | left_ankle_roll_joint         | left_leg   |
| 6     | right_hip_pitch_joint         | right_leg  |
| 7     | right_hip_roll_joint          | right_leg  |
| 8     | right_hip_yaw_joint           | right_leg  |
| 9     | right_knee_pitch_joint        | right_leg  |
| 10    | right_ankle_pitch_joint       | right_leg  |
| 11    | right_ankle_roll_joint        | right_leg  |
| 12    | waist_yaw_joint               | waist      |
| 13    | head_yaw_joint                | head       |
| 14    | head_pitch_joint              | head       |
| 15    | left_shoulder_pitch_joint     | left_manipulator   |
| 16    | left_shoulder_roll_joint      | left_manipulator   |
| 17    | left_shoulder_yaw_joint       | left_manipulator   |
| 18    | left_elbow_pitch_joint        | left_manipulator   |
| 19    | left_wrist_yaw_joint          | left_manipulator   |
| 20    | left_wrist_pitch_joint        | left_manipulator   |
| 21    | left_wrist_roll_joint         | left_manipulator   |
| 22    | right_shoulder_pitch_joint    | right_manipulator  |
| 23    | right_shoulder_roll_joint     | right_manipulator  |
| 24    | right_shoulder_yaw_joint      | right_manipulator  |
| 25    | right_elbow_pitch_joint       | right_manipulator  |
| 26    | right_wrist_yaw_joint         | right_manipulator  |
| 27    | right_wrist_pitch_joint       | right_manipulator  |
| 28    | right_wrist_roll_joint        | right_manipulator  |

### GeneralizedState

| Member Type        | Member Name | Description                                                                 |
|--------------------|-------------|-----------------------------------------------------------------------------|
| `Eigen::VectorXd`  | `q`         | Generalized coordinates: position `(px, py, pz)`, orientation `(qx, qy, qz, qw)`, and joint positions `q_joint` |
| `Eigen::VectorXd`  | `qd`        | Generalized velocities: local linear velocity `v_local`, angular velocity `w_local`, and joint velocities `v_joint` |
| `Eigen::VectorXd`  | `tau`       | Generalized torques: base torque is zero, followed by joint torques `tau_joint` |

### KindynData

| Member Type             | Member Name | Description                                                                 |
|-------------------------|-------------|-----------------------------------------------------------------------------|
| `int`                   | `nv`        | Degrees of freedom (DoF), used as Jacobian column count                     |
| `Vec2<LinkData>`        | `feet_W`    | Feet link kinematic data in world frame                                     |
| `Vec2<LinkData>`        | `hips_W`    | Hips link kinematic data in world frame                                     |
| `Vec2<LinkData>`        | `hands_W`   | Hands link kinematic data in world frame                                    |
| `LinkData`              | `base_W`    | Base link kinematic data in world frame                                     |
| `LinkData`              | `head_W`    | Head link kinematic data in world frame                                     |
| `LinkData`              | `torso_W`   | Torso link kinematic data in world frame                                    |
| `Vec2<LinkData>`        | `feet_B`    | Feet link kinematic data in base frame                                      |
| `Vec2<LinkData>`        | `hips_B`    | Hips link kinematic data in base frame                                      |
| `Vec2<LinkData>`        | `hands_B`   | Hands link kinematic data in base frame                                     |
| `LinkData`              | `base_B`    | Base link kinematic data in base frame                                      |
| `LinkData`              | `head_B`    | Head link kinematic data in base frame                                      |
| `LinkData`              | `torso_B`   | Torso link kinematic data in base frame                                     |
| `JointLimitData`        | `joint_limit` | Joint limit data (vector length `nv - 6`)                                 |
| `DMat<double>`          | `M`         | Generalized mass matrix                                                     |
| `DMat<double>`          | `Minv`      | Inverse of generalized mass matrix                                          |
| `DMat<double>`          | `C`         | Coriolis matrix                                                             |
| `DVec<double>`          | `g`         | Generalized gravity vector                                                  |
| `DVec<double>`          | `c`         | Generalized Coriolis vector                                                 |
| `double`                | `m`         | Total mass of the robot                                                     |
| `Mat3<double>`          | `Iw`        | Inertia matrix in world frame                                               |
| `Vec3<double>`          | `Pc`        | Center of Mass (CoM) position in world frame                                |
| `Vec3<double>`          | `Vc`        | Center of Mass (CoM) linear velocity in world frame                         |
| `Vec3<double>`          | `Wc`        | Center of Mass (CoM) angular velocity in world frame                        |
| `Vec6<double>`          | `hg`        | Centroidal momentum: first 3 linear, last 3 angular                         |
| `DMat<double>`          | `Ag`        | Jacobian of centroidal momentum                                             |
| `DMat<double>`          | `Agd`       | Time derivative of centroidal momentum Jacobian                             |

- `JointLimitData`

| Member Type      | Member Name           | Description                              |
|------------------|-----------------------|------------------------------------------|
| `DVec<double>`   | `lower`               | Lower bounds for each joint              |
| `DVec<double>`   | `upper`               | Upper bounds for each joint              |
| `DVec<double>`   | `effort`              | Maximum effort (torque/force) per joint  |
| `DVec<double>`   | `velocity`            | Maximum velocity for each joint          |

- `LinkData`

| Member Type      | Member Name           | Description                              |
|------------------|-----------------------|------------------------------------------|
| `Vec3<double>`   | `p`                   | Position vector                          |
| `Mat3<double>`   | `R`                   | Rotation matrix                          |
| `Vec3<double>`   | `v`                   | Linear velocity                          |
| `Vec3<double>`   | `w`                   | Angular velocity                         |
| `DMat<double>`   | `J`                   | Jacobian matrix                          |
| `DMat<double>`   | `Jd`                  | Derivative of Jacobian matrix            |

### BaseData

| Member Type          | Member Name     | Description                                                                 |
|----------------------|-----------------|-----------------------------------------------------------------------------|
| `Mat3<double>`        | `R`             | Base to world rotation matrix (base2world)                                   |
| `Vec4<double>`        | `quat_xyzw`     | Quaternion for base to world orientation in XYZW format (base2world)         |
| `Vec4<double>`        | `quat_wxyz`     | Quaternion for base to world orientation in WXYZ format (base2world)         |
| `Vec3<double>`        | `grav_proj`     | Gravity vector in base frame, calculated as `R.transpose() * [0,0,-1]`       |
| `Vec3<double>`        | `rpy`           | ZYX Euler angles: roll, pitch, yaw (in radians)                              |
| `Vec3<double>`        | `omega_W`       | Angular velocity in world frame (in rad/s)                                   |
| `Vec3<double>`        | `acc_W`         | Linear acceleration in world frame (in m/s²)                                 |
| `Vec3<double>`        | `omega_B`       | Angular velocity in base frame (in rad/s)                                    |
| `Vec3<double>`        | `acc_B`         | Linear acceleration in base frame (in m/s²)                                  |
| `Vec3<double>`        | `vel_W`         | Base velocity in world frame (in m/s)                                        |
| `Vec3<double>`        | `pos_W`         | Base position in world frame (in meters)                                     |
| `Vec3<double>`        | `vel_B`         | Base velocity in base frame (in m/s)                                         |

### Contact Data

| Member Type               | Member Name       | Description                             |
|---------------------------|-------------------|-----------------------------------------|
| `Vec2<Vec6<double>>`       | `contact_force`   | 6D contact forces                      |
| `Vec2<double>`             | `contact_prob`    | Contact probability (0~1)              |
