# GR-3 参数

## 关节限位

根据GR-3正式版URDF文件提取出的关节限位。

| 序号 | 序号关节名称                 | 控制组名称        | 位置上限 (rad) | 位置下限 (rad) | 速度上限 (rad/s) | 扭矩上限 (Nm) |
|-------|----------------------------|-------------------|-------------------|-------------------|------------------------|-------------------|
| 0     | left_hip_pitch_joint       | left_leg          | 2.618             | -2.618            | 6.49                   | 366.0             |
| 1     | left_hip_roll_joint        | left_leg          | 1.5708            | -0.17453          | 12.985                 | 140.4             |
| 2     | left_hip_yaw_joint         | left_leg          | 1.5708            | -0.69813          | 12.985                 | 140.4             |
| 3     | left_knee_pitch_joint      | left_leg          | 2.3562            | 0.0               | 6.49                   | 366.0             |
| 4     | left_ankle_pitch_joint     | left_leg          | 0.7854            | -0.7854           | 16.76                  | 59.4              |
| 5     | left_ankle_roll_joint      | left_leg          | 0.43633           | -0.43633          | 16.76                  | 59.4              |
| 6     | right_hip_pitch_joint      | right_leg         | 2.618             | -2.618            | 6.49                   | 366.0             |
| 7     | right_hip_roll_joint       | right_leg         | 0.17453           | -1.5708           | 12.985                 | 140.4             |
| 8     | right_hip_yaw_joint        | right_leg         | 0.69813           | -1.5708           | 12.985                 | 140.4             |
| 9     | right_knee_pitch_joint     | right_leg         | 2.3562            | 0.0               | 6.49                   | 366.0             |
| 10    | right_ankle_pitch_joint    | right_leg         | 0.7854            | -0.7854           | 16.76                  | 59.4              |
| 11    | right_ankle_roll_joint     | right_leg         | 0.43633           | -0.43633          | 16.76                  | 59.4              |
| 12    | waist_yaw_joint            | waist             | 2.618             | -2.618            | 12.985                 | 140.4             |
| 13    | waist_roll_joint           | waist             | 0.17453           | -0.17453          | 14.77                  | 108.6             |
| 14    | waist_pitch_joint          | waist             | 0.5236            | -0.20944          | 14.77                  | 108.6             |
| 15    | head_yaw_joint             | head              | 1.3963            | -1.3963           | 9.21                   | 17.4              |
| 16    | head_pitch_joint           | head              | 0.5236            | -0.5236           | 9.21                   | 17.4              |
| 17    | left_shoulder_pitch_joint  | left_manipulator  | 2.9671            | -2.9671           | 7.75                   | 74.4              |
| 18    | left_shoulder_roll_joint   | left_manipulator  | 1.9199            | -0.2618           | 7.75                   | 74.4              |
| 19    | left_shoulder_yaw_joint    | left_manipulator  | 1.8326            | -1.8326           | 6.28                   | 42.9              |
| 20    | left_elbow_pitch_joint     | left_manipulator  | 0.087266          | -2.2689           | 6.28                   | 42.9              |
| 21    | left_wrist_yaw_joint       | left_manipulator  | 1.8326            | -1.8326           | 6.28                   | 42.9              |
| 22    | left_wrist_pitch_joint     | left_manipulator  | 1.309             | -0.87266          | 9.2153                 | 17.4              |
| 23    | left_wrist_roll_joint      | left_manipulator  | 1.2217            | -1.0472           | 9.2153                 | 17.4              |
| 24    | right_shoulder_pitch_joint | right_manipulator | 2.9671            | -2.9671           | 7.75                   | 74.4              |
| 25    | right_shoulder_roll_joint  | right_manipulator | 0.2618            | -1.9199           | 7.75                   | 74.4              |
| 26    | right_shoulder_yaw_joint   | right_manipulator | 1.8326            | -1.8326           | 6.28                   | 42.9              |
| 27    | right_elbow_pitch_joint    | right_manipulator | 0.087266          | -2.2689           | 6.28                   | 42.9              |
| 28    | right_wrist_yaw_joint      | right_manipulator | 1.8326            | -1.8326           | 6.28                   | 42.9              |
| 29    | right_wrist_pitch_joint    | right_manipulator | 1.309             | -0.87266          | 9.2153                 | 17.4              |
| 30    | right_wrist_roll_joint     | right_manipulator | 1.0472            | -1.2217           | 9.2153                 | 17.4              |
