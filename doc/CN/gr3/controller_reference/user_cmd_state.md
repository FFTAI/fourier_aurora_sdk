# 用户指令状态参考

切换到 **用户指令状态**后，用户可以为所有关节发送外部关节位置命令，以及执行器配置设置命令（如 pd 参数）。机器人将执行这些命令并相应地更新其状态。

## 状态明细

状态名称 | 任务名称     | 手柄映射 | DDS 映射 | 频率
-----------|---------------|------------------|-------------|-------------
UserCmd    | UserCmdTask   | 无               | 10          | 400Hz

可用于悬挂 | 可用于站立 | 自动保护切换
----------------------|------------------------|----------------
是                   | 否                     | 否

## 手柄控制

此状态没有手柄控制。

## 客户端控制

速度控制 | 站姿控制 | 关节控制 | 关节参数控制
-----------------|--------------------|---------------|-------------------
否               | 否                 | 全身    | 全身

### 进入 UserCmd 状态

启动 *AuroraCore* 后，使用 aurora 客户端的 `set_fsm_state` 函数进入用户指令状态。

```python
client = AuroraClient.get_instance(domain_id=123, robot_name="gr3")   # 初始化 aurora 客户端
time.sleep(1)

client.set_fsm_state(10)     # 切换到用户指令状态
```

### 关节控制

可通过 `set_group_cmd` 函数进行关节控制。由于位置命令立即生效，建议在上身关节命令中使用插值以避免命令急剧变化。

**可用控制组：** `left_leg`、`right_leg`、`waist`、`left_manipulator`、`right_manipulator`

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

### 关节参数控制

可通过 `set_motor_cfg_pd` 函数进行关节控制。目前，Aurora 仅支持所有关节的 pd 控制模式。

**可用控制组：** `left_leg`、`right_leg`、`waist`、`left_manipulator`、`right_manipulator`

```python
kp_config = {
    "left_leg": [400, 200, 200, 400, 200, 26],
    "right_leg": [400, 200, 200, 400, 200, 26],
    "waist": [200, 300, 200], 
    "head": [100, 100],
    "left_manipulator": [400, 200, 200, 200, 50, 50, 50],
    "right_manipulator": [400, 200, 200, 200, 50, 50, 50],
}
kd_config = {
    "left_leg": [20, 20, 20, 20, 20, 2.6],
    "right_leg": [20, 20, 20, 20, 20, 2.6],
    "waist": [10, 15, 10],
    "head": [10, 10]
    "left_manipulator": [20, 10, 10, 10, 2.5, 2.5, 2.5],
    "right_manipulator": [20, 10, 10, 10, 2.5, 2.5, 2.5],
}

client.set_motor_cfg_pd(kp_config, kd_config)
```

有关关节限制，请参阅 [机器人规格](../robot_specs.md)
