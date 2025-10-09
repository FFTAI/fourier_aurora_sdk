# GR-1 Robot Specs

## Joint Limits

Joint Limits extracted from the updated GR-1 urdf file.

| Index | Joint Name                 | Group Name        | Upper Limit (rad) | Lower Limit (rad) | Velocity Limit (rad/s) | Torque Limit (Nm) |
|-------|----------------------------|-------------------|-------------------|-------------------|------------------------|-------------------|
| 0     | left_hip_roll_joint        | left_leg          | 0.79              | -0.09             | 7.64                   | 84.18             |
| 1     | left_hip_yaw_joint         | left_leg          | 0.70              | -0.70             | 9.80                   | 65.00             |
| 2     | left_hip_pitch_joint       | left_leg          | 0.70              | -1.75             | 18.54                  | 135.00            |
| 3     | left_knee_pitch_joint      | left_leg          | 1.92              | -0.09             | 18.53                  | 135.00            |
| 4     | left_ankle_pitch_joint     | left_leg          | 0.52              | -1.05             | 10.26                  | 41.67             |
| 5     | left_ankle_roll_joint      | left_leg          | 0.44              | -0.44             | 10.26                  | 41.67             |
| 6     | right_hip_roll_joint       | right_leg         | 0.09              | -0.79             | 7.64                   | 84.18             |
| 7     | right_hip_yaw_joint        | right_leg         | 0.70              | -0.70             | 9.80                   | 65.00             |
| 8     | right_hip_pitch_joint      | right_leg         | 0.70              | -1.75             | 18.54                  | 135.00            |
| 9     | right_knee_pitch_joint     | right_leg         | 1.92              | -0.09             | 18.54                  | 135.00            |
| 10    | right_ankle_pitch_joint    | right_leg         | 0.52              | -1.05             | 10.26                  | 41.67             |
| 11    | right_ankle_roll_joint     | right_leg         | 0.44              | -0.44             | 10.26                  | 41.67             |
| 12    | waist_yaw_joint            | waist             | 1.05              | -1.05             | 9.80                   | 65.00             |
| 13    | waist_pitch_joint          | waist             | 1.22              | -0.52             | 9.80                   | 65.00             |
| 14    | waist_roll_joint           | waist             | 0.70              | -0.70             | 9.80                   | 65.00             |
| 15    | head_roll_joint            | head              | 0.35              | -0.35             | 15.08                  | 5.25              |
| 16    | head_yaw_joint             | head              | 2.71              | -2.71             | 15.08                  | 5.25              |
| 17    | head_pitch_joint           | head              | 0.87              | -0.87             | 15.08                  | 5.25              |
| 18    | left_shoulder_pitch_joint  | left_manipulator  | 1.92              | -2.79             | 6.28                   | 38.46             |
| 19    | left_shoulder_roll_joint   | left_manipulator  | 2.53              | -0.57             | 6.28                   | 38.46             |
| 20    | left_shoulder_yaw_joint    | left_manipulator  | 2.97              | -2.97             | 5.24                   | 30.30             |
| 21    | left_elbow_pitch_joint     | left_manipulator  | 2.27              | -2.27             | 5.24                   | 30.30             |
| 22    | left_wrist_yaw_joint       | left_manipulator  | 2.97              | -2.97             | 16.55                  | 13.17             |
| 23    | left_wrist_roll_joint      | left_manipulator  | 0.96              | -0.87             | 15.08                  | 5.25              |
| 24    | left_wrist_pitch_joint     | left_manipulator  | 0.61              | -0.61             | 15.08                  | 5.25              |
| 25    | right_shoulder_pitch_joint | right_manipulator | 1.92              | -2.79             | 6.28                   | 38.46             |
| 26    | right_shoulder_roll_joint  | right_manipulator | 0.57              | -2.53             | 6.28                   | 38.46             |
| 27    | right_shoulder_yaw_joint   | right_manipulator | 2.97              | -2.97             | 5.24                   | 30.30             |
| 28    | right_elbow_pitch_joint    | right_manipulator | 2.27              | -2.27             | 5.24                   | 30.30             |
| 29    | right_wrist_yaw_joint      | right_manipulator | 2.97              | -2.97             | 16.55                  | 13.17             |
| 30    | right_wrist_roll_joint     | right_manipulator | 0.87              | -0.96             | 15.08                  | 5.25              |
| 31    | right_wrist_pitch_joint    | right_manipulator | 0.61              | -0.61             | 15.08                  | 5.25              |
