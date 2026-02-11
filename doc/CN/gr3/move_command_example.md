# 移动指令示例

本文档介绍如何使用 AuroraClient API 和 MoveCommandManager 发送移动指令来控制机器人。移动指令为关节级和笛卡尔级运动提供高级运动规划，包括轨迹插值、速度控制和运动完成跟踪。

## 移动指令概述

移动指令在特定控制器状态下可用，提供三种运动类型：

- **MOVE_ABS_JOINT (类型 0)**：带轨迹规划的绝对关节位置控制
- **MOVE_JOINT (类型 1)**：使用关节空间插值的笛卡尔位姿控制
- **MOVE_LINE (类型 2)**：沿笛卡尔空间直线路径的笛卡尔线性运动

与直接关节控制不同，移动指令自动处理轨迹插值、速度规划和运动完成检测，非常适合协调多组运动。

**支持移动指令的控制器状态：**

- [RL Locomotion State](controller_reference/rl_locomotion_state.md)，且上半身 FSM 状态设置为 Move Command（状态 4）

## MoveCommandManager

`MoveCommandManager` 类简化了移动指令的创建和管理。它处理消息结构，允许您在发送指令之前配置多个控制组。

**初始化 MoveCommandManager：**

```python
from fourier_aurora_client import MoveCommandManager

# 使用机器人名称初始化
move_command_manager = MoveCommandManager(robot_name='gr3')
```

## 关节级移动指令

### joint_move_command

`joint_move_command()` 函数创建带轨迹规划的绝对关节位置移动指令。这对于以受控速度将关节移动到特定位置非常有用。

**参数：**

- `move_type` (int)：对于 MOVE_ABS_JOINT 必须为 0
- `group_name` (str)：控制组名称（例如 'left_manipulator'、'right_manipulator'）
- `group_pos` (list[float])：控制组的目标关节位置
- `group_vel` (list[float], 可选)：目标关节速度（当前未使用）
- `expect_vel` (int, 可选)：期望速度，以最大速度的千分之几表示（默认值：1000）

**示例 - 绝对关节位置控制：**

```python
# 定义目标关节位置
left_manipulator_target_pos = [0.0, 0.0, 0.0, -1.2, 0.0, 0.0, 0.0]
right_manipulator_target_pos = [0.0, 0.0, 0.0, -1.2, 0.0, 0.0, 0.0]

# 为两个机械臂配置移动指令
move_command_manager.joint_move_command(
    move_type=0,
    group_name="left_manipulator",
    group_pos=left_manipulator_target_pos,
    expect_vel=300  # 最大速度的 30%
)
move_command_manager.joint_move_command(
    move_type=0,
    group_name="right_manipulator",
    group_pos=right_manipulator_target_pos,
    expect_vel=300
)

# 获取配置好的移动指令并发送
move_command = move_command_manager.get_move_command()
client.set_move_command(move_command)
time.sleep(0.2)

# 等待运动完成
occupied_groups = ["left_manipulator", "right_manipulator"]
client.wait_groups_motion_complete(occupied_groups, print_interval=0.3)
```

## 笛卡尔级移动指令

### cartesian_move_command

`cartesian_move_command()` 函数创建笛卡尔空间移动指令。它支持两种运动类型：

- **MOVE_JOINT (类型 1)**：使用关节空间插值将末端执行器移动到目标位姿
- **MOVE_LINE (类型 2)**：在笛卡尔空间中沿直线路径将末端执行器移动到目标位姿

目标位姿指定为 [x, y, z, qx, qy, qz, qw]，其中 (x, y, z) 是位置（单位：米），(qx, qy, qz, qw) 是四元数表示的方向。

**参数：**

- `move_type` (int)：1 表示 MOVE_JOINT，2 表示 MOVE_LINE
- `group_name` (str)：控制组名称
- `group_pos` (list[float])：目标笛卡尔位姿 [x, y, z, qx, qy, qz, qw]
- `group_vel` (list[float], 可选)：目标笛卡尔速度（当前未使用）
- `expect_vel` (int, 可选)：期望速度，以最大速度的千分之几表示（默认值：1000）

**示例 - 关节空间笛卡尔运动：**

```python
# 定义目标位姿（位置 + 四元数方向）
left_arm_pose = [0.3, 0.25, 0.2, 0.0, -0.7071, 0.0, 0.7071]
right_arm_pose = [0.3, -0.25, 0.2, 0.0, -0.7071, 0.0, 0.7071]

# 使用关节空间插值移动
move_command_manager.cartesian_move_command(
    move_type=1,  # MOVE_JOINT
    group_name="left_manipulator",
    group_pos=left_arm_pose,
    expect_vel=300
)
move_command_manager.cartesian_move_command(
    move_type=1,
    group_name="right_manipulator",
    group_pos=right_arm_pose,
    expect_vel=300
)

move_command = move_command_manager.get_move_command()
client.set_move_command(move_command)
time.sleep(0.2)
client.wait_groups_motion_complete(occupied_groups, print_interval=0.3)

# 验证最终位姿
print(f"左侧机械臂位姿: {client.get_cartesian_state('left_manipulator', 'pose')}")
print(f"右侧机械臂位姿: {client.get_cartesian_state('right_manipulator', 'pose')}")
```

**示例 - 笛卡尔线性运动：**

```python
# 定义直线运动的目标位姿
left_arm_pose = [0.3, 0.25, 0.3, 0.0, -0.7071, 0.0, 0.7071]
right_arm_pose = [0.3, -0.25, 0.3, 0.0, -0.7071, 0.0, 0.7071]

# 使用笛卡尔线性路径移动
move_command_manager.cartesian_move_command(
    move_type=2,  # MOVE_LINE
    group_name="left_manipulator",
    group_pos=left_arm_pose,
    expect_vel=300
)
move_command_manager.cartesian_move_command(
    move_type=2,
    group_name="right_manipulator",
    group_pos=right_arm_pose,
    expect_vel=300
)

move_command = move_command_manager.get_move_command()
client.set_move_command(move_command)
time.sleep(0.2)
client.wait_groups_motion_complete(occupied_groups, print_interval=0.3)
```

## 运动完成跟踪

### wait_groups_motion_complete

`wait_groups_motion_complete()` 函数会阻塞执行，直到所有指定的控制组完成运动。这对于顺序移动指令至关重要。

```python
# 定义参与运动的所有控制组
occupied_groups = ["waist", "head", "left_manipulator", "right_manipulator"]

# 等待所有控制组完成运动
client.wait_groups_motion_complete(
    occupied_groups, 
    print_interval=0.3  # 每 0.3 秒打印一次状态
)
```

或者，使用 `get_group_motion_state()` 检查单个控制组：

```python
# 检查特定控制组是否完成运动
motion_state = client.get_group_motion_state('left_manipulator')
# 返回值：0 表示运动完成，非零表示仍在移动
```

## 安全注意事项

- **控制器状态**：在发送移动指令之前，务必设置正确的 FSM 状态（RL Locomotion）和上半身 FSM 状态（Move Command）
- **速度限制**：使用适当的 `expect_vel` 值。测试时建议从较低值（200-400）开始
- **运动完成**：在发送下一条指令之前，始终等待运动完成
- **关节限位**：验证目标位置在关节限位范围内
- **碰撞避免**：确保目标位姿不会导致自碰撞或工作空间违规
- **坐标系**：笛卡尔位姿在机器人基座坐标系中指定

## 完整移动指令示例

以下是演示所有三种移动指令类型的完整示例：

```python
from fourier_aurora_client import AuroraClient
from fourier_aurora_client import MoveCommandManager
import time

# Initialize client
client = AuroraClient.get_instance(domain_id=136, robot_name='gr3')
move_command_manager = MoveCommandManager(robot_name='gr3')
occupied_groups = ["waist", "head", "left_manipulator", "right_manipulator"]
print_interval = 0.3
print("Initializing robot for joint control...")

# Step 1: Set FSM state to RL locomotion State
cmd = input("Press Enter to set FSM to RL locomotion (state 3)...")
client.set_fsm_state(3)
time.sleep(0.5)
client.set_upper_fsm_state(4)

# Step 2: Set Move Abs Joint Command
cmd = input("Press Enter to send move abs joint command...")
print("Sending move abs joint command...")

left_manipulator_target_pos =  [0.0, 0.0, 0.0, -1.2, 0.0, 0.0, 0.0]
right_manipulator_target_pos = [0.0, 0.0, 0.0, -1.2, 0.0, 0.0, 0.0]

move_command_manager.joint_move_command(
    move_type=0,
    group_name="left_manipulator",
    group_pos=left_manipulator_target_pos,
    expect_vel=300
)
move_command_manager.joint_move_command(
    move_type=0,
    group_name="right_manipulator",
    group_pos=right_manipulator_target_pos,
    expect_vel=300
)

move_command = move_command_manager.get_move_command()
client.set_move_command(move_command)
time.sleep(0.2)
client.wait_groups_motion_complete(occupied_groups, print_interval=print_interval)
time.sleep(0.5)

print(f"left_manipulator_position: {client.get_group_state('left_manipulator', 'position')}")
print(f"right_manipulator_position: {client.get_group_state('right_manipulator', 'position')}")
print(f"left_manipulator_pose: {client.get_cartesian_state('left_manipulator', 'pose')}")
print(f"right_manipulator_pose: {client.get_cartesian_state('right_manipulator', 'pose')}")

#Step 3: Set Move Joint Command
cmd = input("Press Enter to send move joint command...")
print("Sending move joint command...")
left_arm_pre_pose =  [0.3,  0.25, 0.2, 0.0, -0.7071, 0.0, 0.7071]
right_arm_pre_pose = [0.3, -0.25, 0.2, 0.0, -0.7071, 0.0, 0.7071]

move_command_manager.cartesian_move_command(
    move_type=1,
    group_name="left_manipulator",
    group_pos=left_arm_pre_pose,
    expect_vel=300
)
move_command_manager.cartesian_move_command(
    move_type=1,
    group_name="right_manipulator",
    group_pos=right_arm_pre_pose,
    expect_vel=300
)

move_command = move_command_manager.get_move_command()
client.set_move_command(move_command)
time.sleep(0.2)
client.wait_groups_motion_complete(occupied_groups, print_interval=print_interval)
time.sleep(0.5)

print(f"left_manipulator_pose: {client.get_cartesian_state('left_manipulator', 'pose')}")
print(f"right_manipulator_pose: {client.get_cartesian_state('right_manipulator', 'pose')}")

#Step 4: Set Move Joint Command
cmd = input("Press Enter to send move line command...")
print("Sending move line command...")
left_arm_pre_pose =  [0.3,  0.25, 0.3, 0.0, -0.7071, 0.0, 0.7071]
right_arm_pre_pose = [0.3, -0.25, 0.3, 0.0, -0.7071, 0.0, 0.7071]

move_command_manager.cartesian_move_command(
    move_type=2,
    group_name="left_manipulator",
    group_pos=left_arm_pre_pose,
    expect_vel=300
)
move_command_manager.cartesian_move_command(
    move_type=2,
    group_name="right_manipulator",
    group_pos=right_arm_pre_pose,
    expect_vel=300
)

move_command = move_command_manager.get_move_command()
client.set_move_command(move_command)
time.sleep(0.2)
client.wait_groups_motion_complete(occupied_groups, print_interval=print_interval)
time.sleep(0.5)

print(f"left_manipulator_pose: {client.get_cartesian_state('left_manipulator', 'pose')}")
print(f"right_manipulator_pose: {client.get_cartesian_state('right_manipulator', 'pose')}")

client.close()
```

该示例演示了移动指令控制的完整工作流程：设置 FSM 状态、使用绝对关节指令、笛卡尔关节空间插值和笛卡尔线性运动，并进行适当的运动完成跟踪。
