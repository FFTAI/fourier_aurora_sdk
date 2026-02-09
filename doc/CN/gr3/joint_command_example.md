# 关节命令用例

本文档说明如何使用 AuroraClient API 发送关节级命令来控制机器人。Aurora 通过控制组提供直接的关节位置、速度和扭矩控制，以及电机配置调整功能。所有命令必须通过正确定义的控制组发送，以确保协调运动。

## 控制单元

Aurora 不支持对执行器的直接控制；所有关节控制都与 URDF 模型对齐。对于并联机构，计算在 Aurora 内部完成，只有关节级状态和命令可用。要实现执行器级控制，请参阅 *fourier_actuator_sdk*。

**控制组**代表一组连续的可控关节，是 Aurora 中的重要概念。对于 GR-3，控制组包括：`left_leg`、`right_leg`、`waist`、`left_manipulator`、`right_manipulator`。Aurora 中的最小控制单元是控制组，这意味着开发者每次必须为完整的控制组发送命令。

**重要说明：**

- 命令必须包含控制组中的所有关节
- 关节顺序必须与控制组定义匹配
- 发送到单个关节的命令将被拒绝
- 实时控制需要正确的 FSM 状态配置

有关关节和控制组的详细规格，请参阅 [机器人规格](robot_specs.md)

## 关节控制

### set_group_cmd

`set_group_cmd()` 函数向一个或多个控制组发送位置、速度和扭矩命令。每个命令的位置命令是必需的，而速度和扭矩命令默认为 0。Aurora 中的每个控制器允许不同的控制组接收外部命令，如下表所列。未列出的控制器无法接收任何组命令。

控制器    | left_leg | right_leg | head | waist | left_manipulator | right_manipulator
----------|----------|-----------|-----|-------|------------------|-------------
[PdStand 状态](controller_reference/pd_stand_state.md) | ❌ | ❌ | ✅ | ✅ | ✅ | ✅
[RL 运动状态](controller_reference/rl_locomotion_state.md) | ❌ | ❌ | ✅ | ✅ | ✅ | ✅
[UserCmd 状态](controller_reference/user_cmd_state.md) | ✅ | ✅ | ✅ | ✅ | ✅ | ✅
[Upper UserCmd 状态](controller_reference/upper_user_cmd_state.md) | ❌ | ❌ | ✅ | ✅ | ✅ | ✅

Aurora 默认运行 PD 控制模式。其扭矩输出可以通过以下公式计算：

$$
\tau = K_{p}(q_{cmd}-q) + K_{d}(\dot{q}_{cmd}-\dot{q}) + \tau_{cmd}
$$

有关执行器控制回路的详细信息，请参阅 *fourier_actuator_sdk*。

**示例 - 直接位置控制：**

```python
left_manipulator_pos = [0.0, 0.0, 0.0, -0.2, 0.0, 0.0, 0.0]

# 控制左机械臂 - 移动到特定关节位置
position_cmd = {
    'left_manipulator': left_manipulator_pos
}

client.set_group_cmd(position_cmd=position_cmd)
print("左臂位置命令已发送")
```

**示例 - 多组插值控制：**

```python
# 使用插值运动同时控制多个组
target_pos = {
    "left_manipulator_target" = [0.0, 0.0, 0.0, -1.0, 0.0, 0.0, 0.0],
    "right_manipulator_target" = [0.0, 0.0, 0.0, -1.0, 0.0, 0.0, 0.0],
    "waist_target" = [0.5, 0.0, 0.0]
}

init_pos = {
    "left_manipulator_target" = client.get_group_state("left_manipulator", key="position"),
    "right_manipulator_target" = client.get_group_state("right_manipulator", key="position"),
    "waist_target" = client.get_group_state("waist", key="position"),
}

total_step = 200
dt = 0.01
for step in range(total_step):
    pos_cmd = {}
    for control_group in target_pos.keys():
        group_pos_cmd = [init + (target - init) * step / total_step for init, target in zip(init_pos[control_group], target_pos[control_group])]
        pos_cmd[control_group] = group_pos_cmd

    client.set_group_cmd(position_cmd=pos_cmd)
    time.sleep(dt)
print("多个控制组已命令到目标位置")
```

**安全注意事项：**

- 在发送命令之前始终验证关节限制
- 应用插值以避免位置突然变化
- 在运动期间监控机器人状态

## 电机配置

### set_motor_cfg_pd

`set_motor_cfg_pd()` 函数为指定控制组中的电机配置 PD（比例-微分）控制增益。正确的增益调整对于稳定和准确的运动控制至关重要。

**默认配置：**
Aurora 为每个控制组使用预调的默认增益。每个控制器都带有预调的电机配置。某些控制器允许外部电机配置设置以供开发者使用。下表列出了每个控制器中设置电机配置的可用性。

控制器    | left_leg | right_leg | head | waist | left_manipulator | right_manipulator
--------------|----------|-------|-------|-------|------------------|-------------
[PdStand 状态](controller_reference/pd_stand_state.md) | ❌ | ❌ | ❌ | ❌ | ❌ | ❌
[RL 运动状态](controller_reference/rl_locomotion_state.md) | ❌ | ❌ | ❌ | ❌ | ❌ | ❌
[UserCmd 状态](controller_reference/user_cmd_state.md) | ✅ | ✅ | ✅ | ✅ | ✅ | ✅
[Upper UserCmd 状态](controller_reference/upper_user_cmd_state.md) | ❌ | ❌ | ✅ | ✅ | ✅ | ✅

**示例 - 设置自定义 PD 增益：**

```python
# 使用中等增益配置左机械臂以进行柔顺控制
# 值是控制组中每个关节的值
left_manipulator_kp = [400, 200, 200, 200, 50, 50, 50]
left_manipulator_kd = [20, 10, 10, 10, 2.5, 2.5, 2.5]

kp_config = {'left_manipulator': left_manipulator_kp}
kd_config = {'left_manipulator': left_manipulator_kd}

client.set_motor_cfg_pd(kp_config=kp_config, kd_config=kd_config)
print("左臂 PD 增益已配置为柔顺控制")

# 读回配置
actual_kp = client.get_group_motor_cfg('left_manipulator', 'pd_kp')
actual_kd = client.get_group_motor_cfg('left_manipulator', 'pd_kd')

print(f"配置的 Kp: {kp_config['left_manipulator']}")
print(f"实际 Kp: {actual_kp}")
print(f"配置的 Kd: {kd_config['left_manipulator']}")
print(f"实际 Kd: {actual_kd}")
```

## 完整的关节控制示例

以下是演示电机配置和关节控制的完整示例：

```python
from fourier_aurora_client import AuroraClient
import time

# 初始化客户端
client = AuroraClient.get_instance(domain_id=123)

print("正在初始化机器人进行关节控制...")

# 步骤 1: 将 FSM 状态设置为用户命令状态
client.set_fsm_state(10)
time.sleep(0.5)

# 步骤 2: 配置电机增益
print("正在配置电机 PD 增益...")
kp_config = {
    'left_manipulator': [400, 200, 200, 200, 50, 50, 50],
    'right_manipulator': [400, 200, 200, 200, 50, 50, 50],
    "waist": [200, 300, 200], 
    "head": [100, 100],
}
kd_config = {
    'left_manipulator': [20, 10, 10, 10, 2.5, 2.5, 2.5],
    'right_manipulator': [20, 10, 10, 10, 2.5, 2.5, 2.5],
    "waist": [10, 15, 10],
    "head": [10, 10],
}
client.set_motor_cfg_pd(kp_config=kp_config, kd_config=kd_config)
time.sleep(0.2)

# 步骤 3: 获取当前位置
print("正在读取当前位置...")
current_pos = {
    'left_manipulator': client.get_group_state('left_manipulator', key='position'),
    'right_manipulator': client.get_group_state('right_manipulator', key='position'),
    'waist': client.get_group_state('waist', key='position')
}
print(f"  左机械臂: {[f'{p:.3f}' for p in current_pos['left_manipulator']]}")
print(f"  右机械臂: {[f'{p:.3f}' for p in current_pos['right_manipulator']]}")
print(f"  腰部: {[f'{p:.3f}' for p in current_pos['waist']]}")

# 步骤 4: 定义目标位置
target_pos = {
    'left_manipulator': [0.0, 0.0, 0.0, -1.0, 0.0, 0.0, 0.0],
    'right_manipulator': [0.0, 0.0, 0.0, -1.0, 0.0, 0.0, 0.0],
    'waist': [0.5, 0.0, 0.0]
}

# 步骤 5: 使用插值移动到目标位置
print("\n正在移动到目标位置...")
total_steps = 200
dt = 0.01

for step in range(total_steps + 1):
    pos_cmd = {}
    for group_name in target_pos.keys():
        # 从当前位置到目标位置的线性插值
        group_pos_cmd = [
            curr + (targ - curr) * step / total_steps 
            for curr, targ in zip(current_pos[group_name], target_pos[group_name])
        ]
        pos_cmd[group_name] = group_pos_cmd
    
    client.set_group_cmd(position_cmd=pos_cmd)
    time.sleep(dt)

print("已到达目标位置")
time.sleep(1.0)

# 步骤 6: 使用插值返回零位置
print("\n正在返回零位置...")
zero_pos = {
    'left_manipulator': [0.0] * 7,
    'right_manipulator': [0.0] * 7,
    'waist': [0.0] * 3
}

# 获取当前位置（应该在目标位置）
start_pos = {
    'left_manipulator': client.get_group_state('left_manipulator', key='position'),
    'right_manipulator': client.get_group_state('right_manipulator', key='position'),
    'waist': client.get_group_state('waist', key='position')
}

for step in range(total_steps + 1):
    pos_cmd = {}
    for group_name in zero_pos.keys():
        # 从当前位置到零位置的线性插值
        group_pos_cmd = [
            start + (zero - start) * step / total_steps 
            for start, zero in zip(start_pos[group_name], zero_pos[group_name])
        ]
        pos_cmd[group_name] = group_pos_cmd
    
    client.set_group_cmd(position_cmd=pos_cmd)
    time.sleep(dt)

print("已到达零位置")
time.sleep(0.5)

# 步骤 7: 监控最终状态
print("\n最终机器人状态:")
final_left = client.get_group_state('left_manipulator', 'position')
final_right = client.get_group_state('right_manipulator', 'position')
final_waist = client.get_group_state('waist', 'position')
print(f"  左机械臂: {[f'{p:.3f}' for p in final_left]}")
print(f"  右机械臂: {[f'{p:.3f}' for p in final_right]}")
print(f"  腰部: {[f'{p:.3f}' for p in final_waist]}")

print("\n关节控制演示完成")
client.close()
```

此示例演示了关节控制的完整工作流程：配置 FSM 状态、设置电机增益、读取当前位置、使用平滑插值移动到目标位置、返回零位置以及监控最终状态。
