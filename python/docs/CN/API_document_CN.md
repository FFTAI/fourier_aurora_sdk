# Fourier Aurora 客户端 API 文档

## 函数列表

- [get_instance](#get_instance)
- [close](#close)
- [get_fsm_state](#get_fsm_state)
- [get_fsm_name](#get_fsm_name)
- [get_upper_fsm_state](#get_upper_fsm_state)
- [get_upper_fsm_name](#get_upper_fsm_name)
- [get_velocity_source](#get_velocity_source)
- [get_velocity_source_name](#get_velocity_source_name)
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
- [set_group_cmd](#set_group_cmd)
- [set_motor_cfg_pd](#set_motor_cfg_pd)

## 用户接口

### get_instance

```python
AuroraClient.get_instance(domain_id: int , participant_qos = None, robot_name: Optional[str]= None, namespace: Optional[str] = None, is_ros_compatible: bool = False) -> AuroraClient
```

初始化一个 AuroraClient 实例。

- 参数
  - domain_id (*int*)：DDS 域的 domain ID。需与 Aurora 服务器的 domain ID 保持一致。
  - participant_qos (*fastdds.DomainParticipantQos, 可选*)：DDS DomainParticipant 的服务质量（QoS）设置。
  - robot_name (*str, 可选*)：机器人的名称。
  - namespace (*str, 可选*)：ROS 类型前缀之后的命名空间前缀。
  - is_ros_compatible (*bool, 可选*)：为客户端添加 ROS 类型名称修饰。

- 返回值
  - AuroraClient：AuroraClient 类的一个实例。

### close

```python
AuroraClient.close()
```

关闭 AuroraClient 实例。

## Getter 函数

### get_fsm_state

```python
AuroraClient.get_fsm_state() -> int
```

获取机器人的当前状态。

- 返回值
  - int：当前全身 FSM 状态的 id。可能的返回值：
    - 0 - 默认状态
    - 1 - 关节站立状态
    - 2 - PD 站立状态
    - 3 - RL 运动状态
    - 9 - 安全保护状态
    - 10 - 用户指令状态
    - 11 - 上半身用户指令状态

### get_fsm_name

```python
AuroraClient.get_fsm_name() -> str
```

获取机器人的当前状态。

- 返回值：
  - str：当前全身 FSM 状态的名称。

### get_upper_fsm_state

```python
AuroraClient.get_upper_fsm_state() -> int
```

返回当前上半身 FSM 状态名称。

- 返回值
  - int：当前上半身 FSM 状态的 id。可能的返回值：
    - 0 - 默认状态
    - 1 - 动作状态（摆臂）
    - 2 - 远程状态
    - 4 - 规划运动状态

### get_upper_fsm_name

```python
AuroraClient.get_upper_fsm_name() -> str
```

返回当前上半身 FSM 状态名称。

- 返回值
  - str：当前上半身 FSM 状态的名称。

### get_velocity_source

```python
AuroraClient.get_velocity_source() -> int
```

获取机器人当前的速度指令来源。

- 返回值
  - int：当前速度指令来源的 id。可能的返回值：
    - 0 - 手柄
    - 2 - 导航（DDS）

### get_velocity_source_name

```python
AuroraClient.get_velocity_source_name() -> str
```

获取机器人当前的速度指令来源。

- 返回值
  - str：当前速度指令来源的名称。

### get_stand_pose

```python
AuroraClient.get_stand_pose() -> list
```

获取机器人的站立姿态。

- 返回值
  - list[float]：站立姿态数据列表，格式为 [delta_z, delta_pitch, delta_yaw, stable_level]

### get_group_state

```python
AuroraClient.get_group_state(group_name: str, key: str = 'position') -> list[float]
```

获取指定机器人控制组的状态

- 参数
  - group_name (*str*)：机器人控制组的名称。
  - key (*str, 可选*)：需要从控制组状态中获取的键。有效值：
    - 'position' - 关节位置
    - 'velocity' - 关节速度
    - 'effort' - 关节力矩

- 返回值
  - list[float]：指定控制组所请求的状态数据。

### get_cartesian_state

```python
AuroraClient.get_cartesian_state(group_name: str, key: str = 'pose') -> list[float]
```

获取指定机器人控制组的笛卡尔状态。

- 参数
  - group_name (*str*)：机器人控制组的名称。
  - key (*str, 可选*)：需要从笛卡尔状态中获取的键。有效值：
    - 'pose' - 控制组末端位姿
    - 'twist' - 控制组末端速度
    - 'wrench' - 控制组末端力矩

- 返回值
  - list[float]：指定控制组所请求的笛卡尔状态数据。

### get_base_data

```python
AuroraClient.get_base_data(key: str) -> list[float]
```

获取机器人的基座数据。

- 参数
  - key (*str*)：需要从基座数据中获取的键。
    - 'quat_xyzw' - 四元数，格式为 x, y, z, w
    - 'quat_wxyz' - 四元数，格式为 w, x, y, z
    - 'rpy' - 滚转、俯仰、偏航，单位为弧度
    - 'omega_W' - 世界坐标系下的角速度
    - 'acc_W' - 世界坐标系下的线加速度
    - 'omega_B' - 基座坐标系下的角速度
    - 'acc_B' - 基座坐标系下的线加速度
    - 'vel_W' - 世界坐标系下的线速度
    - 'pos_W' - 世界坐标系下的位置
    - 'vel_B' - 基座坐标系下的线速度

- 返回值
  - list[float]：所请求的基座数据。

### get_contact_data

```python
AuroraClient.get_contact_data(key: str) -> list[float]
```

获取机器人的接触数据。

- 参数
  - key (*str*)：需要从接触数据中获取的键。
    - 'contact_fz' - 接触力和力矩
    - 'contact_prob' - 接触概率

- 返回值
  - list[float]：所请求的接触数据。

## Setter 函数

### set_fsm_state

```python
AuroraClient.set_fsm_state(state: int)
```

设置机器人的全身 FSM 状态。

- 参数
  - state (*int*)：机器人的目标状态。有效值：
    - 0 - 默认状态
    - 1 - 关节站立状态
    - 2 - PD 站立状态
    - 3 - RL 运动状态
    - 9 - 安全保护状态
    - 10 - 用户指令状态
    - 11 - 上半身用户指令状态

### set_upper_fsm_state

```python
AuroraClient.set_upper_fsm_state(state: int)
```

设置机器人的上半身 FSM 状态。

- 参数
  - state (*int*)：机器人的目标上半身状态。有效值：
    - 0 - 默认状态
    - 1 - 动作状态（摆臂）
    - 2 - 远程状态
    - 4 - 规划运动状态

### set_velocity_source

```python
AuroraClient.set_velocity_source(source: int)
```

设置速度指令的来源。

- 参数
  - source (*int*)：目标速度指令来源。有效值：
    - 0 - 手柄
    - 2 - 导航（DDS）

- 注意
  - 若需客户端控制速度，请将来源设置为 2（导航）。

### set_velocity

```python
AuroraClient.set_velocity(vx: float, vy: float, yaw: float, duration: float = 1.0)
```

设置机器人的速度指令。

- 参数
  - vx (*float*)：x 方向线速度（前进/后退），单位为 m/s
  - vy (*float*)：y 方向线速度（左/右），单位为 m/s
  - yaw (*float*)：绕 z 轴的角速度（旋转），单位为 rad/s
  - duration (*float, 可选*)：速度指令的有效持续时间，单位为秒。

- 注意
  - 实际达到的速度可能受到机器人物理能力的限制。

### set_stand_pose

```python
AuroraClient.set_stand_pose(delta_z: float, delta_pitch: float, delta_yaw: float)
```

设置机器人的站立姿态指令。

- 参数
  - delta_z (*float*)：基座高度的期望变化量（单位：米）。
                     正值抬高机器人，负值降低机器人。
  - delta_pitch (*float*)：俯仰角的期望变化量（单位：弧度）。
                         正值使机器人前倾，负值使机器人后仰。
  - delta_yaw (*float*)：偏航角的期望变化量（单位：弧度）。
                       正值使机器人左转，负值使机器人右转。

- 注意
  - 站立姿态调整仅在 PD 站立状态（2）下可用。实际站立姿态可能受到机器人物理能力的限制。

### set_group_cmd

```python
AuroraClient.set_group_cmd(position_cmd: Dict[str, list[float]], velocity_cmd: Optional[Dict[str, list[float]]] = None, torque_cmd: Optional[Dict[str, list[float]]] = None):
```

设置机器人的关节位置。

- 参数
  - position_dict (*Dict[str, list[float]]*)：以关节名称为键、关节位置为值的字典。
  - velocity_dict (*Dict[str, list[float]], 可选*)：以关节名称为键、关节速度为值的字典。
  - torque_dict (*Dict[str, list[float]], 可选*)：以关节名称为键、关节力矩为值的字典。

### set_motor_cfg_pd

```python
AuroraClient.set_motor_cfg_pd(self, kp_config: Dict[str, list[float]], kd_config:Dict[str, list[float]])
```

设置机器人的电机配置。

- 参数
  - kp_config (*Dict[str, list[float]]*)：以控制组名称为键、比例增益（kp）值为列表的字典。
  - kd_config (*Dict[str, list[float]]*)：以控制组名称为键、微分增益（kd）值为列表的字典。

- 注意
  - kp_config 和 kd_config 中的键应与机器人电机配置中的控制组名称匹配。
  - kp_config 和 kd_config 中的值应为表示控制组中每个关节增益的浮点数列表。