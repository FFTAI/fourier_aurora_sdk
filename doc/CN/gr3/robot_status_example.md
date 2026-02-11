# 机器人状态获取用例

本文档说明如何使用 AuroraClient API 获取各种机器人状态信息。机器人提供实时状态数据，包括基座运动数据、接触信息、关节状态和不同控制组的笛卡尔状态。本文档中列出的所有机器人状态在整个运行时都可用。

## 基座数据

### get_base_data

`get_base_data` 函数在基座坐标和世界坐标参考系中检索机器人的基座运动数据。这包括方向（作为四元数或欧拉角）、速度、加速度和位置。

**坐标系定义：**  
**基座坐标系：** 机器人坐标系建立在机器人基座中心。x 轴指向基座的前方向，y 轴指向左方向，z 轴指向正上方。x、y 和 z 轴遵循右手定则分布。  
**世界坐标系：** 世界坐标系建立在 Aurora 启动时机器人基座中心的地面投影点。x、y 和 z 轴的方向与世界坐标系对齐。

**可用键：**

- `'quat_xyzw'` - x, y, z, w 格式的四元数
- `'quat_wxyz'` - w, x, y, z 格式的四元数
- `'rpy'` - 滚转、俯仰、偏航（弧度）
- `'omega_W'` - 世界坐标系中的角速度
- `'acc_W'` - 世界坐标系中的线性加速度
- `'omega_B'` - 基座坐标系中的角速度
- `'acc_B'` - 基座坐标系中的线性加速度
- `'vel_W'` - 世界坐标系中的线性速度
- `'pos_W'` - 世界坐标系中的位置
- `'vel_B'` - 基座坐标系中的线性速度

**示例：**

```python
# 获取四元数形式的基座方向 (x, y, z, w)
quat = client.get_base_data('quat_xyzw')
print(f"基座方向 (四元数): {quat}")

# 获取滚转、俯仰、偏航形式的基座方向
rpy = client.get_base_data('rpy')
print(f"基座方向 (RPY): roll={rpy[0]:.3f}, pitch={rpy[1]:.3f}, yaw={rpy[2]:.3f}")

# 获取世界坐标系中的线性速度
vel_world = client.get_base_data('vel_W')
print(f"基座速度 (世界坐标系): {vel_world}")

# 获取世界坐标系中的位置
pos_world = client.get_base_data('pos_W')
print(f"基座位置 (世界坐标系): {pos_world}")
```

## 接触数据

### get_contact_data

Aurora 使用基于广义动量的扰动观测器估计接触力信息，并将这些扰动分解为单个脚接触力和概率。开发者可以使用 `get_contact_data` 函数来检索接触力和概率。

**示例：**

```python
# 获取接触力
contact_forces = client.get_contact_data('contact_fz')
print(f"接触力: {contact_forces}")

# 获取接触概率（用于确定脚是否在地面上）
contact_probs = client.get_contact_data('contact_prob')
print(f"接触概率: {contact_probs}")

# 检查左脚和右脚是否接触
if len(contact_probs) >= 2:
    left_foot_contact = contact_probs[0] > 0.5
    right_foot_contact = contact_probs[1] > 0.5
    print(f"左脚接触: {left_foot_contact}, 右脚接触: {right_foot_contact}")
```

## 关节状态

### get_group_state

`get_group_state()` 函数检索特定控制组的关节级状态信息（位置、速度或力）。

**示例：**

```python
# 获取左机械臂控制组位置
left_manipulator_pos = client.get_group_state('left_manipulator', 'position')
print(f"左机械臂关节位置: {left_manipulator_pos}")

# 获取右机械臂控制组速度
right_manipulator_vel = client.get_group_state('right_manipulator', 'velocity')
print(f"右机械臂关节速度: {right_manipulator_vel}")

# 获取头部控制组力
head_effort = client.get_group_state('head', 'effort')
print(f"头部关节力: {head_effort}")

# 监控控制组的所有关节状态
print("\n完整的左机械臂状态:")
print(f"  位置: {client.get_group_state('left_manipulator', 'position')}")
print(f"  速度: {client.get_group_state('left_manipulator', 'velocity')}")
print(f"  力: {client.get_group_state('left_manipulator', 'effort')}")
```

## 笛卡尔状态

### get_cartesian_state

`get_cartesian_state()` 函数检索特定控制组末端执行器的笛卡尔空间信息（位姿、扭转或扳手）。

**示例：**

```python
# 获取左机械臂末端执行器位姿
left_manipulator_pose = client.get_cartesian_state('left_manipulator', 'pose')
print(f"左机械臂末端执行器位姿: {left_manipulator_pose}")
if len(left_manipulator_pose) >= 7:
    print(f"  位置: x={left_manipulator_pose[0]:.3f}, y={left_manipulator_pose[1]:.3f}, z={left_manipulator_pose[2]:.3f}")
    print(f"  方向 (四元数): qx={left_manipulator_pose[3]:.3f}, qy={left_manipulator_pose[4]:.3f}, qz={left_manipulator_pose[5]:.3f}, qw={left_manipulator_pose[6]:.3f}")

# 获取右机械臂末端执行器扭转（速度）
right_manipulator_twist = client.get_cartesian_state('right_manipulator', 'twist')
print(f"\n右机械臂末端执行器扭转: {right_manipulator_twist}")
if len(right_manipulator_twist) >= 6:
    print(f"  线性速度: vx={right_manipulator_twist[0]:.3f}, vy={right_manipulator_twist[1]:.3f}, vz={right_manipulator_twist[2]:.3f}")
    print(f"  角速度: wx={right_manipulator_twist[3]:.3f}, wy={right_manipulator_twist[4]:.3f}, wz={right_manipulator_twist[5]:.3f}")

# 获取左机械臂末端执行器扳手（力/扭矩）
left_manipulator_wrench = client.get_cartesian_state('left_manipulator', 'wrench')
print(f"\n左机械臂末端执行器扳手: {left_manipulator_wrench}")
if len(left_manipulator_wrench) >= 6:
    print(f"  力: fx={left_manipulator_wrench[0]:.3f}, fy={left_manipulator_wrench[1]:.3f}, fz={left_manipulator_wrench[2]:.3f}")
    print(f"  扭矩: tx={left_manipulator_wrench[3]:.3f}, ty={left_manipulator_wrench[4]:.3f}, tz={left_manipulator_wrench[5]:.3f}")
```

## 电机配置

### get_group_motor_cfg

`get_group_motor_cfg()` 函数检索特定控制组的电机配置参数。这包括不同控制模式的 PID 增益。

注意：目前 Aurora 在开源版本中仅提供 PD 控制模式。因此，此函数仅对 `pd_kp` 和 `pd_kd` 有效。

**示例：**

```python
# 获取左机械臂的 PD 控制模式增益
left_manipulator_kp = client.get_group_motor_cfg('left_manipulator', 'pd_kp')
left_manipulator_kd = client.get_group_motor_cfg('left_manipulator', 'pd_kd')
print(f"左机械臂 PD 增益:")
print(f"  Kp: {left_manipulator_kp}")
print(f"  Kd: {left_manipulator_kd}")
```

## 完整的状态监控示例

以下是在循环中监控各种机器人状态的完整示例：

```python
from fourier_aurora_client import AuroraClient
import time

# Initialize client
client = AuroraClient.get_instance(domain_id=123, robot_name='gr3')

print("Starting robot status monitoring...")
print("Press Ctrl+C to stop\n")

try:
    while True:
        # Base status
        rpy = client.get_base_data('rpy')
        vel_world = client.get_base_data('vel_W')
        
        # Contact status
        contact_probs = client.get_contact_data('contact_prob')
        
        # Left manipulator status
        left_manipulator_pos = client.get_group_state('left_manipulator', 'position')
        left_manipulator_pose = client.get_cartesian_state('left_manipulator', 'pose')
        
        print(f"[{time.strftime('%H:%M:%S')}] Robot Status:")
        print(f"  Base RPY: [{rpy[0]:.3f}, {rpy[1]:.3f}, {rpy[2]:.3f}]")
        print(f"  Base Vel: [{vel_world[0]:.3f}, {vel_world[1]:.3f}, {vel_world[2]:.3f}]")
        print(f"  Contact: {['Yes' if p > 0.5 else 'No' for p in contact_probs]}")
        print(f"  Left manipulator Joints: {len(left_manipulator_pos)} DOF")
        print(f"  Left manipulator EE Pos: [{left_manipulator_pose[0]:.3f}, {left_manipulator_pose[1]:.3f}, {left_manipulator_pose[2]:.3f}]")
        print("-" * 60)
        
        time.sleep(0.5)  # Update every 0.5 seconds
        
except KeyboardInterrupt:
    print("\nStopping monitoring...")
    client.close()
```

此监控脚本提供机器人基座运动、接触状态和机械臂位置的实时视图，这对于在操作期间调试和理解机器人行为很有用。
