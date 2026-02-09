# Pd站立状态参考

**Pd站立状态**运行一个 WBC 站立控制器，使机器人站立在平坦表面上以进行进一步控制。pdstand 控制器中有两个阶段，*悬挂阶段*和*站立阶段*。阶段变化由内部稳定状态参估计器决定，用户可以通过客户端获取。

如果稳定水平低于 100（对于仿真，低于 10），控制器将切换到*悬挂阶段*。在*悬挂阶段*，所有关节将缓慢移回默认位置，并且站姿或关节控制不可用。

如果稳定水平高于 100（对于仿真，高于 10），控制器将切换到*站立阶段*。在*站立阶段*，可以调整站姿和控制上身关节。

## 状态明细

状态名称 | 任务名称     | 手柄映射 | DDS 映射 | 频率
-----------|---------------|------------------|-------------|-------------
PdStand    | PdStandTask   | LB+B             | 2           | 400Hz

可用于悬挂 | 可用于站立 | 自动保护切换
----------------------|------------------------|----------------
是                   | 是                    | 否

## 手柄控制

### 进入Pd站立状态

启动 *AuroraCore* 后，同时按下肩键 `LB` 和按钮 `B` 进入Pd站立状态。

### 站姿控制

机器人进入站立阶段后，使用手柄上的方向键来控制机器人的站姿。

- 按 `上` 或 `下` 方向键来控制机器人的高度。
- 按 `左` 或 `右` 方向键来控制机器人的俯仰角。
- 将右摇杆向 `左` 或 `右` 移动来控制机器人的偏航角。

### 重置按钮

Pd站立状态提供快捷键来重置机器人的状态。长按按钮 `A`，机器人将

1. 将站姿重置为默认姿势。
2. 逐渐将手臂移动到默认位置。

## 客户端控制

速度控制 | 站姿控制 | 关节控制 | 关节参数控制
-----------------|--------------------|---------------|-------------------
否               | 是                | 上身    | 否

### 进入Pd站立状态

启动 *AuroraCore* 后，使用 aurora 客户端的 `set_fsm_state` 函数进入Pd站立状态。

```python
client = AuroraClient.get_instance(domain_id=123, robot_name="gr3")   # 初始化 aurora 客户端
time.sleep(1)

client.set_fsm_state(2)     # 切换到Pd站立状态
```

### 站姿控制

机器人进入站立阶段后，可通过 `set_stand_pose` 函数进行站姿控制。

**站姿范围：** `delta_z`: [0.01, -0.20], `delta_pitch`: [-0.3, 0.6], `delta_yaw`: [-0.3, 0.3]

```python
client.set_stand_pose(-0.1, 0.0, 0.0)   # 下蹲 0.1 米
time.sleep(2.0)

client.set_stand_pose(0.0, 0.2, 0.0)    # 前倾 0.2 弧度
time.sleep(2.0)

client.set_stand_pose(0.0, 0.0, 0.2)    # 左转 0.2 弧度
time.sleep(2.0)

client.set_stand_pose(0.0, 0.0, 0.0)    # 返回默认姿势
time.sleep(2.0)
```

### 关节控制

一旦机器人进入站立阶段，可通过 `set_group_cmd` 函数进行关节控制。由于位置命令立即生效，建议在上身关节命令中使用插值以避免命令急剧变化。

**可用控制组：** `Waist`、`Head`、`Left_Manipulator`、`Right_Manipulator`

```python
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
