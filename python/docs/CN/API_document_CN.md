# Fourier Aurora 客户端 API 文档

## 函数列表

- [get_instance](#get_instance)
- [close](#close)
- [get_fsm_state](#get_fsm_state)
- [get_upper_fsm_state](#get_upper_fsm_state)
- [get_velocity_source](#get_velocity_source)
- [get_stand_pose](#get_stand_pose)
- [get_group_state](#get_group_state)
- [get_cartesian_state](#get_cartesian_state)
- [get_base_data](#get_base_data)
- [get_contact_data](#get_contact_data)
- [set_fsm_state](#set_fsm_state)
- [set_upper_fsm_state](#set_upper_fsm_state)
- [set_velocity_source](#set_velocity_source)
- [set_velocity](#set_velocity)
- [set_stand_pose](#set_stand_pose)
- [set_joint_positions](#set_joint_positions)
- [set_motor_cfg](#set_motor_cfg)

## 用户接口

### get_instance

```python
def get_instance(cls, domain_id: int =123, participant_qos = 0, robot_name: str = "gr2t2v2", serial_number = None) -> "AuroraClient":
```

初始化一个 AuroraClient 实例。

- 参数
  - domain_id (int)：DDS 域的 domain ID，需与 Aurora 服务器保持一致。
  - participant_qos (int)：DDS DomainParticipant 的服务质量（QoS）设置。
  - robot_name (str)：机器人的名称。
  - serial_number：机器人的序列号。

- 返回值
  - AuroraClient：AuroraClient 类的一个实例。

### close

```python
def close(self)
```

关闭 AuroraClient 实例。

## Getter 函数

### get_fsm_state

```python
def get_fsm_state(self) -> str:
```

获取机器人当前的全身 FSM 状态。

- 返回值
  - str：当前全身 FSM 状态名称。可能的返回值：
    - 0 - 默认状态
    - 1 - 关节站立
    - 2 - PD 站立
    - 3~8 - 用户控制器 A~F
    - 9 - 安全保护
    - 10 - 用户指令
    - 11 - 上半身用户指令

### get_upper_fsm_state

```python
def get_upper_fsm_state(self) -> str:
```

获取当前上半身 FSM 状态名称。

- 返回值
  - str：当前上半身 FSM 状态名称。可能的返回值：
    - 0 - 默认状态
    - 1 - 动作状态（摆臂）
    - 2 - 远程状态

### get_velocity_source

```python
def get_velocity_source(self) -> str:
```

获取机器人当前的速度指令来源。

- 返回值
  - str：当前速度指令来源。可能的返回值：
    - 0 - 手柄
    - 1 - 手持设备
    - 2 - 导航

### get_stand_pose

```python
def get_stand_pose(self) -> list
```

获取机器人的站立姿态。

- 返回值
  - list[float]：站立姿态数据，格式为 [delta_z, delta_pitch, delta_yaw, stable_level]

### get_group_state

```python
def get_group_state(self, group_name: str, key: str = 'position') -> list[float]
```

获取指定控制组的状态。

- 参数
  - group_name (str)：机器人控制组名称。
  - key (str)：需要获取的状态键。有效值：
    - 'position' - 关节位置
    - 'velocity' - 关节速度
    - 'effort' - 关节力矩

- 返回值
  - list[float]：指定控制组的状态数据。

### get_cartesian_state

```python
def get_cartesian_state(self, group_name: str, key: str = 'pose') -> list[float]
```

获取指定控制组的笛卡尔状态。

- 参数
  - group_name (str)：机器人控制组名称。
  - key (str)：需要获取的状态键。有效值：
    - 'pose' - 姿态
    - 'twist' - 速度
    - 'wrench' - 力矩

- 返回值
  - list[float]：指定组的笛卡尔状态数据。

### get_base_data

```python
def get_base_data(self, key: str) -> list[float]
```

获取机器人的基础数据。

- 参数
  - key (str)：需要获取的基础数据键。有效值：
    - 'quat_xyzw' - 四元数 (x, y, z, w)
    - 'quat_wxyz' - 四元数 (w, x, y, z)
    - 'rpy' - 欧拉角 (roll, pitch, yaw)
    - 'omega_W' - 世界坐标系下的角速度
    - 'acc_W' - 世界坐标系下的加速度
    - 'omega_B' - 基座坐标系下的角速度
    - 'acc_B' - 基座坐标系下的加速度
    - 'vel_W' - 世界坐标系下的线速度
    - 'pos_W' - 世界坐标系下的位置
    - 'vel_B' - 基座坐标系下的线速度

- 返回值
  - list[float]：所需的基础数据。

### get_contact_data

```python
def get_contact_data(self, key: str) -> list[float]
```

获取机器人的接触数据。

- 参数
  - key (str)：需要获取的接触数据键。有效值：
    - 'contact_fz' - 接触力和力矩
    - 'contact_prob' - 接触概率

- 返回值
  - list[float]：所需的接触数据。

## Setter 函数

### set_fsm_state

```python
def set_fsm_state(self, state: int)
```

设置机器人的全身 FSM 状态。

- 参数
  - state (int)：目标状态。有效值：
    - 0 - 默认状态
    - 1 - 关节站立
    - 2 - PD 站立
    - 3~8 - 用户控制器 A~F
    - 9 - 安全保护
    - 10 - 用户指令
    - 11 - 上半身用户指令

### set_upper_fsm_state

```python
def set_upper_fsm_state(self, state: int)
```

设置机器人的上半身 FSM 状态。

- 参数
  - state (int)：目标上半身状态。有效值：
    - 0 - 默认状态
    - 1 - 动作状态（摆臂）
    - 2 - 远程状态

### set_velocity_source

```python
def set_velocity_source(self, source: int)
```

设置速度指令的来源。

- 参数
  - source (int)：目标速度指令来源。有效值：
    - 0 - 手柄
    - 1 - 手持设备
    - 2 - 导航

- 注意
  - 若需客户端控制速度，请设置为 2（导航）。

### set_velocity

```python
def set_velocity(self, vx: float, vy: float, yaw: float)
```

设置机器人的速度指令。

- 参数
  - vx (float)：x 方向线速度（前/后），单位 m/s
  - vy (float)：y 方向线速度（左/右），单位 m/s
  - yaw (float)：绕 z 轴的角速度（旋转），单位 rad/s

- 注意
  - 实际速度可能受到机器人物理能力的限制。

### set_stand_pose

```python
def set_stand_pose(self, delta_z: float, delta_pitch: float, delta_yaw: float)
```

设置机器人的站立姿态指令。

- 参数
  - delta_z (float)：基座高度的变化量（米）。正数抬高，负数降低。
  - delta_pitch (float)：俯仰角的变化量（弧度）。正数前倾，负数后仰。
  - delta_yaw (float)：偏航角的变化量（弧度）。正数左转，负数右转。

- 注意
  - 姿态调整仅在 PD 站立状态（2）下有效，且可能受到机器人物理能力的限制。

### set_joint_positions

```python
def set_joint_positions(self, position_dict: Dict[str, list[float]], is_upper: bool = True)
```

设置机器人的关节位置。

- 参数
  - position_dict (Dict[str, list[float]])：以关节名称为键，关节位置为值的字典。

### set_motor_cfg

```python
def set_motor_cfg(self, kp_config: Dict[str, list[float]], kd_config: Dict[str, list[float]])
```

设置机器人的电机控制参数。

- 参数
  - kp_config (Dict[str, list[float]])：以控制组名称为键，比例增益 (kp) 为值的列表。
  - kd_config (Dict[str, list[float]])：以控制组名称为键，微分增益 (kd) 为值的列表。

- 注意
  - kp_config 与 kd_config 的键必须与机器人电机配置中的组名一致。
  - kp_config 与 kd_config 的值必须是包含每个关节增益的浮点数列表。