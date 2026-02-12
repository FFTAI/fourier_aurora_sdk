# RL行走状态参考

**RL行走状态**允许机器人在下身移动的同时在上身的摆臂和关节控制之间切换。下身将执行基于强化学习的控制器，允许在三个方向上进行速度控制。

另一方面，上身由状态管理器任务管理，允许用户在不同的上身控制器之间切换。可用的控制器有：

- Default：无控制器，映射到上身状态 0。
- UpperBodyActTask：摆臂任务，映射到上身状态 1。
- UpperBodyTeleTask：关节控制任务，映射到上身状态 2。

## 状态明细

状态名称 | 任务名称                                      | 手柄映射 | DDS 映射 | 频率
-----------|------------------------------------------------|------------------|-------------|-------------
RL Locomotion  | LowerBodyWbcrlTask / UpperBodyStateManagerTask   | RB+A             | 3           | 50Hz / 500Hz

可用于悬挂 | 可用于站立 | 自动保护切换
----------------------|------------------------|----------------
否                    | 是                    | 否

## 手柄控制

### 进入RL行走状态

启动 *AuroraCore* 后，同时按下肩键 `RB` 和按钮 `A` 进入RL行走状态。

### 速度控制

使用左右摇杆对机器人应用速度控制。

- 左摇杆垂直轴：向前和向后移动
- 左摇杆水平轴：向左和向右移动
- 右摇杆水平轴：向左和向右转

### 摆臂切换

上身状态管理器提供快捷键来打开摆臂任务。单击按钮 `B` 打开摆臂，然后单击 `X` 关闭摆臂。

## 客户端控制

速度控制 | 站姿控制 | 关节控制 | 关节参数控制
-----------------|--------------------|---------------|-------------------
是              | 否                 | 上身    | 否

### 进入RL行走状态

启动 *AuroraCore* 后，使用 aurora 客户端的 `set_fsm_state` 函数进入RL行走状态。

```python
client = AuroraClient.get_instance(domain_id=123, robot_name="gr2")   # 初始化 aurora 客户端
time.sleep(1)

client.set_fsm_state(3)     # 切换到RL行走状态
```

### 速度控制

在通过客户端应用速度控制之前，首先需要在 Aurora 中切换速度源。速度源确保一次只有一个源可以发送速度命令，以确保安全。可以使用 `set_velocity_source` 函数设置速度源，其中 0 表示手柄控制，2 表示客户端控制。

先将速度源设置为 2，之后通过 `set_velocity` 函数进行速度控制。

**速度范围：** `vx`: [-0.5, 0.75], `vy`: [-0.5, 0.5], `vyaw`: [-1.0, 1.0]

```python
client.set_velocity_source(2)   # 将速度源设置为客户端控制
time.sleep(0.5)

client.set_velocity(0.3, 0.0, 0.0, 5.0)  # 使机器人以 0.3 m/s 向前移动 5 秒
time.sleep(5.0)

client.set_velocity(0.0, 0.3, 0.0, 5.0)  # 使机器人以 0.3 m/s 向左移动 5 秒
time.sleep(5.0)

client.set_velocity(0.0, 0.0, 0.5, 5.0)  # 使机器人以 0.5 rad/s 左转 5 秒
time.sleep(5.0)

client.set_velocity(0.0, 0.0, 0.0, 1.0)  # 使机器人停止
```

### 摆臂

要打开摆臂任务，将上身状态更改为 1。

```python
client.set_upper_fsm_state(1)   # 打开摆臂
```

### 关节控制

要在 RL locomotion 状态下应用关节控制，需要将上身状态更改为 2。

一旦上身状态更改为 2，可通过 `set_group_cmd` 函数进行关节控制。由于位置命令立即生效，建议在上身关节命令中使用插值以避免命令急剧变化。

**可用控制组：** `waist`、`head`、`left_manipulator`、`right_manipulator`

```python
client.set_upper_fsm_state(2)   # 打开关节控制

left_manipulator_init_pose = client.get_group_state("left_manipulator", key="position")
left_manipulator_target_pose = [0.0, 0.0, 0.0, -1.2, 0.0, 0.0, 0.0]
total_steps = 200

for i in range(total_steps):
    # 从初始位置插值到目标姿势
    left_manipulator_pose = [i + (t - i) * i / total_steps for i, t in zip(left_manipulator_init_pose, left_manipulator_target_pose)]
    client.set_group_cmd({"left_manipulator": left_manipulator_pose})
    time.sleep(0.01)
```

有关关节限制，请参阅 [机器人规格](../robot_specs.md)

### 机械臂移动指令

移动指令为关节级和笛卡尔级运动提供高级运动规划，包括轨迹插值、速度控制和运动完成跟踪。与直接关节控制不同，移动指令自动处理轨迹插值、速度规划和运动完成检测，非常适合协调多组运动。

有关机械臂移动指令的详细信息和示例，请参阅 [机械臂移动指令示例](../move_command_example.md)
