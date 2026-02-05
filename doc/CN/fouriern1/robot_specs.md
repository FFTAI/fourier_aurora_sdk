# Fourier N-1 机器人规格

## 关节限制

从原始 Fourier N-1 urdf 文件中提取的关节限制。

| 索引 | 关节名称                 | 控制组名称        | 上限 (rad) | 下限 (rad) | 速度限制 (rad/s) | 扭矩限制 (Nm) |
|-------|----------------------------|-------------------|-------------------|-------------------|------------------------|-------------------|
|     1 | left_hip_pitch_joint       | left_leg          | 2.617             | -2.617            | 16.2                   | 90                |
|     2 | left_hip_roll_joint        | left_leg          | 1.57              | -0.261            | 14.738                 | 54                |
|     3 | left_hip_yaw_joint         | left_leg          | 2.617             | -2.617            | 14.738                 | 54                |
|     4 | left_knee_pitch_joint      | left_leg          | 2.356             | -0.0872           | 16.2                   | 90                |
|     5 | left_ankle_roll_joint      | left_leg          | 0.436             | -0.436            | 16.747                 | 30                |
|     6 | left_ankle_pitch_joint     | left_leg          | 0.436             | -0.436            | 16.747                 | 30                |
|     7 | right_hip_pitch_joint      | right_leg         | 2.617             | -2.617            | 16.2                   | 90                |
|     8 | right_hip_roll_joint       | right_leg         | 0.261             | -1.57             | 14.738                 | 54                |
|     9 | right_hip_yaw_joint        | right_leg         | 2.617             | -2.617            | 14.738                 | 54                |
|    10 | right_knee_pitch_joint     | right_leg         | 2.356             | -0.0872           | 16.2                   | 90                |
|    11 | right_ankle_roll_joint     | right_leg         | 0.436             | -0.436            | 16.747                 | 30                |
|    12 | right_ankle_pitch_joint    | right_leg         | 0.436             | -0.436            | 16.747                 | 30                |
|    13 | waist_yaw_joint            | waist             | 2.617             | -2.617            | 14.738                 | 54                |
|    14 | left_shoulder_pitch_joint  | left_manipulator  | 2.966             | -2.966            | 14.738                 | 54                |
|    15 | left_shoulder_roll_joint   | left_manipulator  | 2.792             | -0.174            | 16.747                 | 30                |
|    16 | left_shoulder_yaw_joint    | left_manipulator  | 1.832             | -1.832            | 16.747                 | 30                |
|    17 | left_elbow_pitch_joint     | left_manipulator  | 1.658             | -0.349            | 16.747                 | 30                |
|    18 | left_wrist_yaw_joint       | left_manipulator  | 1.832             | -1.832            | 16.747                 | 30                |
|    19 | right_shoulder_pitch_joint | right_manipulator | 2.966             | -2.966            | 14.738                 | 54                |
|    20 | right_shoulder_roll_joint  | right_manipulator | 0.174             | -2.792            | 16.747                 | 30                |
|    21 | right_shoulder_yaw_joint   | right_manipulator | 1.832             | -1.832            | 16.747                 | 30                |
|    22 | right_elbow_pitch_joint    | right_manipulator | 1.658             | -0.349            | 16.747                 | 30                |
|    23 | right_wrist_yaw_joint      | right_manipulator | 1.832             | -1.832            | 16.747                 | 30                |
