# 运动命令用例

本文档说明如何使用 AuroraClient API 发送高级运动命令来控制机器人。运动命令包括站姿调整和速度控制，每个命令仅在特定的控制器状态下可用。

## 站姿控制

### set_stand_pose

`set_stand_pose()` 函数在 PD 站立控制器中调整机器人站姿。对于 `PdStand` 状态，站姿调整仅在站立阶段可用。`get_stand_pose()` 函数读取当前站姿（高度、俯仰、偏航），以便您可以验证调整。

**可用于站姿调整的任务：** [PdStand 状态](controller_reference/pd_stand_state.md)

**示例 - 调整站姿：**

```python
client.set_fsm_state(2)     # 切换到允许站姿调整的 pdstand 状态
time.sleep(1.0)

client.set_stand_pose(-0.1, 0.0, 0.0)   # 下蹲 0.1 米
time.sleep(2.0)
print(client.get_stand_pose())          # 打印当前站姿

client.set_stand_pose(0.0, 0.2, 0.0)    # 前倾 0.2 弧度
time.sleep(2.0)
print(client.get_stand_pose())          # 打印当前站姿

client.set_stand_pose(0.0, 0.0, 0.2)    # 左转 0.2 弧度
time.sleep(2.0)
print(client.get_stand_pose())          # 打印当前站姿

client.set_stand_pose(0.0, 0.0, 0.0)    # 返回默认姿势
time.sleep(2.0)
print(client.get_stand_pose())          # 打印当前站姿
```

## 速度控制

### set_velocity_source

在发送速度命令之前，切换速度源，以便只有一个设备控制机器人。使用 `set_velocity_source()`，其中 `0` 表示手柄控制，`2` 表示客户端控制。

### set_velocity

`set_velocity()` 函数在 RL 运动控制器中提供 x、y 方向和偏航旋转的速度控制。持续时间选项设置速度命令应用的时间，机器人将在持续时间结束后逐渐减速。

**可用于速度命令的任务：** [RL 运动状态](controller_reference/rl_locomotion_state.md)

**示例 - 速度命令：**

```python
client.set_fsm_state(3)         # 切换到允许速度控制的 rl locomotion 状态
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

## 安全注意事项

- 在发送命令之前始终切换到正确的控制器状态
- 设置速度源以避免与手柄控制的命令冲突
- 调整站姿时使用小增量以避免突然运动
- 在运动命令期间监控机器人状态

## 完整的运动命令示例

以下是演示站姿和速度控制的完整示例：

```python
from fourier_aurora_client import AuroraClient
import time

# 初始化客户端
client = AuroraClient.get_instance(domain_id=123)

print("正在初始化运动控制演示...")

# 步骤 1: 将 FSM 状态设置为用户命令状态
cmd = input("按 Enter 将 FSM 设置为 PdStand 状态（状态 2）...")
client.set_fsm_state(2)
time.sleep(1.0)

# 步骤 2: PD 站姿调整
cmd = input("按 Enter 开始下蹲和站立...")
print("将站姿调整到比默认姿势低 0.10 米")
client.set_stand_pose(-0.10, 0.0, 0.0)
time.sleep(2.0)
print(f"当前站姿: {client.get_stand_pose()}")

print("将站姿调整回默认姿势")
client.set_stand_pose(0.0, 0.0, 0.0)
time.sleep(2.0)

# 步骤 3: 将 FSM 状态设置为 Rl Locomotion 状态
cmd = input("按 Enter 将 FSM 设置为 Rl Locomotion 状态（状态 3）...")
client.set_fsm_state(3)
client.set_velocity_source(2)
time.sleep(0.5)

# 步骤 3: 设置速度命令
cmd = input("按 Enter 设置速度命令...")
print("正在发送速度命令...")
print("以 0.2 m/s 向前移动")
client.set_velocity(0.2, 0.0, 0.0, 5.0)
time.sleep(3.0)

print("以 0.2 m/s 向左移动")
client.set_velocity(0.0, 0.2, 0.0, 5.0)
time.sleep(3.0)

print("以 0.4 rad/s 逆时针旋转")
client.set_velocity(0.0, 0.0, 0.4, 5.0)
time.sleep(3.0)

print("停止")
client.set_velocity(0.0, 0.0, 0.0, 1.0)
time.sleep(1.0)

print("运动命令演示完成")
client.close()
```

此示例演示了站姿和速度控制的使用：切换到 pdstand 状态后发送站姿调整命令；切换到 rl locomotion 状态后发送速度命令。
