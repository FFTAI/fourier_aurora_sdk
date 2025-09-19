# RobotData 数据结构参考

本文档介绍所有任务间共享的 RobotData 数据结构。

`RobotData`包含多个子数据结构，涵盖机器人状态和控制的各个方面。以下是各子数据结构的简要说明，包括其成员变量及用于访问和修改内容的函数。

## Struct: `RobotData`

| 成员类型               | 成员名称          | 描述                                 |
|------------------------|------------------|--------------------------------------|
| `JoyStickData`         | `joyStick`       | 手柄控制输入数据                     |
| `FsmStateData`         | `fsmState`       | 有限状态机(FSM)状态                  |
| `FourierHardwareData`  | `hardwareData`   | Fourier硬件层数据                    |
| `GeneralizedState`     | `generState`     | 广义机器人状态                       |
| `KindynData`           | `kindynData`     | 运动学与动力学相关数据               |
| `BaseData`             | `baseData`       | 机器人基座数据(位姿/速度等)          |
| `ContactData`          | `contactData`    | 接触力信息(足部接触等)               |

### JoyStickData

| 成员类型             | 成员名称      | 描述                    |
|----------------------|--------------|-------------------------|
| `Eigen::Vector3d`    | `liner_cmd`  | 三维速度指令(x,y,z)     |

### FourierHardwareData

| 返回类型                        | 函数名称与参数                                                                | 说明                           |
|---------------------------------|-------------------------------------------------------------------------------|--------------------------------|
| `grx_sot::robot::RobotState`    | `getRobotStateFixed()`                                                        | 获取机器人当前状态             |
| `bool`                          | `setWholeBodyPosCmd(const Eigen::VectorXd& pos_cmd)`                          | 设置全身关节位置指令           |
| `bool`                          | `setControlGroupPosCmd(std::string group_name, Eigen::VectorXd pos_cmd)`      | 设置指定关节组位置指令         |
| `int`                           | `setWholeBodyPDModePD(const Eigen::VectorXd& kp, const Eigen::VectorXd& kd)`  | 设置全身PD增益                 |
| `bool`                          | `setControlGroupPDModePD(std::string group_name, const Eigen::VectorXd& kp, kd)` | 设置指定关节组PD增益        |
| `void`                          | `getWholeBodyPDModePD(Eigen::VectorXd& kp, Eigen::VectorXd& kd)`              | 获取当前全身PD增益             |
| `void`                          | `getControlGroupPDModePD(std::string group_name, Eigen::VectorXd& kp, kd)`    | 获取指定关节组PD增益           |
| `std::vector<std::string>`      | `getHandNameList()`                                                           | 获取灵巧手名称列表             |
| `int`                           | `setHandPVC(std::string hand_name, std::vector<float> pos_cmd, std::vector<float> vel_cmd, std::vector<float> acc_cmd)` | 设置指定灵巧手位置，速度和加速度指令   |
| `std::vector<std::vector<float>>`  | `getHandPVC(std::string hand_name)`                                        | 获取灵巧手当前位置，速度和加速度  |

- `RobotState`

| 成员类型           | 成员名称    | 描述           |
|--------------------|-------------|----------------|
| `Eigen::VectorXd`  | `q`         | 关节位置       |
| `Eigen::VectorXd`  | `qd`        | 关节速度       |
| `Eigen::VectorXd`  | `qdd`       | 关节加速度     |
| `Eigen::VectorXd`  | `qft`       | 关节力/扭矩    |

- 关节顺序

| 序号 | 关节名称                    | 所属组别           |
|------|-----------------------------|--------------------|
| 0    | left_hip_pitch_joint        | left_leg           |
| 1    | left_hip_roll_joint         | left_leg           |
| 2    | left_hip_yaw_joint          | left_leg           |
| 3    | left_knee_pitch_joint       | left_leg           |
| 4    | left_ankle_pitch_joint      | left_leg           |
| 5    | left_ankle_roll_joint       | left_leg           |
| 6    | right_hip_pitch_joint       | right_leg          |
| 7    | right_hip_roll_joint        | right_leg          |
| 8    | right_hip_yaw_joint         | right_leg          |
| 9    | right_knee_pitch_joint      | right_leg          |
| 10   | right_ankle_pitch_joint     | right_leg          |
| 11   | right_ankle_roll_joint      | right_leg          |
| 12   | waist_yaw_joint             | waist              |
| 13   | head_yaw_joint              | head               |
| 14   | head_pitch_joint            | head               |
| 15   | left_shoulder_pitch_joint   | left_manipulator   |
| 16   | left_shoulder_roll_joint    | left_manipulator   |
| 17   | left_shoulder_yaw_joint     | left_manipulator   |
| 18   | left_elbow_pitch_joint      | left_manipulator   |
| 19   | left_wrist_yaw_joint        | left_manipulator   |
| 20   | left_wrist_pitch_joint      | left_manipulator   |
| 21   | left_wrist_roll_joint       | left_manipulator   |
| 22   | right_shoulder_pitch_joint  | right_manipulator  |
| 23   | right_shoulder_roll_joint   | right_manipulator  |
| 24   | right_shoulder_yaw_joint    | right_manipulator  |
| 25   | right_elbow_pitch_joint     | right_manipulator  |
| 26   | right_wrist_yaw_joint       | right_manipulator  |
| 27   | right_wrist_pitch_joint     | right_manipulator  |
| 28   | right_wrist_roll_joint      | right_manipulator  |

### GeneralizedState

| 成员类型           | 成员名称    | 描述                                                                 |
|--------------------|-------------|----------------------------------------------------------------------|
| `Eigen::VectorXd`  | `q`         | 广义坐标: 位置  `(px, py, pz)`, 朝向 `(qx, qy, qz, qw)`, 和关节位置 `q_joint` |
| `Eigen::VectorXd`  | `qd`        | 广义速度: 基座线速度  `v_local`, 角速度  `w_local`, 和关节速度  `v_joint`     |
| `Eigen::VectorXd`  | `tau`       | 广义力矩: 基座力矩（0）, 关节力矩 `tau_joint`                                 |

### KindynData

| 成员类型           | 成员名称      | 描述                                               |
|--------------------|---------------|----------------------------------------------------|
| `int`              | `nv`          | 自由度数量，用于雅可比列数计算                     |
| `Vec2<LinkData>`   | `feet_W`      | 世界坐标系下双足运动学数据                         |
| `Vec2<LinkData>`   | `hips_W`      | 世界坐标系下双髋运动学数据                         |
| `Vec2<LinkData>`   | `hands_W`     | 世界坐标系下双手运动学数据                         |
| `LinkData`         | `base_W`      | 世界坐标系下基座运动学数据                         |
| `LinkData`         | `head_W`      | 世界坐标系下头部运动学数据                         |
| `LinkData`         | `torso_W`     | 世界坐标系下躯干运动学数据                         |
| `Vec2<LinkData>`   | `feet_B`      | 基座坐标系下双足运动学数据                         |
| `Vec2<LinkData>`   | `hips_B`      | 基座坐标系下双髋运动学数据                         |
| `Vec2<LinkData>`   | `hands_B`     | 基座坐标系下双手运动学数据                         |
| `LinkData`         | `base_B`      | 基座坐标系下基座运动学数据                         |
| `LinkData`         | `head_B`      | 基座坐标系下头部运动学数据                         |
| `LinkData`         | `torso_B`     | 基座坐标系下躯干运动学数据                         |
| `JointLimitData`   | `joint_limit` | 关节活动度极限数据(长度等于`nv - 6`)               |
| `DMat<double>`     | `M`           | 广义质量矩阵                                       |
| `DMat<double>`     | `Minv`        | 逆广义质量矩阵                                     |
| `DMat<double>`     | `C`           | 科里奥利矩阵                                       |
| `DVec<double>`     | `g`           | 广义重力矩阵                                       |
| `DVec<double>`     | `c`           | 广义科里奥利向量                                   |
| `double`           | `m`           | 机器人总质量                                       |
| `Mat3<double>`     | `Iw`          | 世界坐标系下惯量矩阵                               |
| `Vec3<double>`     | `Pc`          | 世界坐标系下质心位置                               |
| `Vec3<double>`     | `Vc`          | 世界坐标系下质心速度                               |
| `Vec3<double>`     | `Wc`          | 世界坐标系下质心加速度                             |
| `Vec6<double>`     | `hg`          | 质心动量: 前三个线动量, 后三个角动量               |
| `DMat<double>`     | `Ag`          | 质心动量雅可比矩阵                                 |
| `DMat<double>`     | `Agd`         | 质心动量雅可比矩阵的时间导数                       |

- `JointLimitData`

| 成员类型         | 成员名称    | 描述               |
|------------------|-------------|--------------------|
| `DVec<double>`   | `lower`     | 各关节活动度下限   |
| `DVec<double>`   | `upper`     | 各关节活动度上限   |
| `DVec<double>`   | `effort`    | 各关节最大力矩     |
| `DVec<double>`   | `velocity`  | 各关节最高速度     |

- `LinkData`

| 成员类型         | 成员名称    | 描述               |
|------------------|-------------|--------------------|
| `Vec3<double>`   | `p`         | 位置向量           |
| `Mat3<double>`   | `R`         | 旋转矩阵           |
| `Vec3<double>`   | `v`         | 线速度             |
| `Vec3<double>`   | `w`         | 角速度             |
| `DMat<double>`   | `J`         | 雅可比矩阵         |
| `DMat<double>`   | `Jd`        | 雅可比矩阵的导数   |

### BaseData

| 成员类型         | 成员名称      | 描述                                                         |
|------------------|---------------|--------------------------------------------------------------|
| `Mat3<double>`   | `R`           | 基座到世界坐标系的旋转矩阵 (base2world)                      |
| `Vec4<double>`   | `quat_xyzw`   | 基座到世界坐标系的xyzw格式的四元数朝向  (base2world)         |
| `Vec4<double>`   | `quat_wxyz`   | 基座到世界坐标系的wxyz格式的四元数朝向  (base2world)         |
| `Vec3<double>`   | `grav_proj`   | 基座坐标系下的重力矩阵, 计算方式为 `R.transpose() * [0,0,-1]`|
| `Vec3<double>`   | `rpy`         | ZYX格式的欧拉角: 翻滚，俯仰，偏航角 (in radians)             |
| `Vec3<double>`   | `omega_W`     | 世界坐标系下的角速度 (in rad/s)                              |
| `Vec3<double>`   | `acc_W`       | 世界坐标系下的线加速度 (in m/s²)                             |
| `Vec3<double>`   | `omega_B`     | 基座坐标系下的角速度 (in rad/s)                              |
| `Vec3<double>`   | `acc_B`       | 基座坐标系下的线加速度 (in m/s²)                             |
| `Vec3<double>`   | `vel_W`       | 世界坐标系下基座的速度 (in m/s)                              |
| `Vec3<double>`   | `pos_W`       | 世界坐标系下基座的位置 (in meters)                           |
| `Vec3<double>`   | `vel_B`       | 基座坐标系下基座的速度 (in m/s)                              |

### Contact Data

| 成员类型                | 成员名称        | 描述               |
|-------------------------|-----------------|--------------------|
| `Vec2<Vec6<double>>`    | `contact_force` | 6维接触力/力矩     |
| `Vec2<double>`          | `contact_prob`  | 接触概率 (0~1)     |
