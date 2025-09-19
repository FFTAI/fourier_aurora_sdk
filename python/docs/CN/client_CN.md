
## 函數列表

- [get_instance](#get_instance)
- [close](#close)
- [get_fsm_state](#get_fsm_state)
- [get_upper_fsm_state](#get_upper_fsm_state)
- [get_velocity_source](#get_velocity_source)
- [get_stand_pose](#get_stand_pose)
- [get_group_state](#get_group_state)
- [get_cartesian_state](#get_cartesian_state)
- [get_base_data](#get_base_data)
- [get_true_data](#get_true_data)
- [get_contact_data](#get_contact_data)
- [set_fsm_state](#set_fsm_state)
- [set_upper_fsm_state](#set_upper_fsm_state)
- [set_velocity_source](#set_velocity_source)
- [set_velocity](#set_velocity)
- [set_stand_pose](#set_stand_pose)
- [set_joint_positions](#set_joint_positions)
- [set_motor_cfg](#set_motor_cfg)

# User Interface

## get_instance

```python
def get_instance(cls, domain_id: int =123, participant_qos = 0, robot_name: str = "gr2t2v2", serial_number = None) -> "AuroraClient":
```
初始化AuroraClient实例。
### 参数：
    domain_id(int): DDS域的ID。应与Aurora服务器的域ID匹配。
    participant_qos(int): DDS DomainParticipant的QoS设置。
    robot_name(str): 机器人名称。
    serial_number: 机器人序列号。

### 返回值：
    AuroraClient: AuroraClient类的实例。

## close

```python
def close(self)
```

关闭AuroraClient实例。


# Getter Function 

## get_fsm_state

```python
def get_fsm_state(self) -> str:
```

获得机器人的当前状态。

### 返回值:
    字符串: 当前状态的名称。可能的值为:
        0 - 默认状态
        1 - 关节站立
        2 - PD站立
        3~8 - 用户控制器A~F
        9 - 安全保护
        10 - 用户命令
        11 - 上半身用户命令


## get_upper_fsm_state

```python
def get_upper_fsm_state(self) -> str:
```

获得机器人的当前上半身状态。

### 返回值: 
    str: 当前上半身状态的名称。可能的值为:
        0 - 默认状态
        1 - 摆臂
        2 - 远程控制


## get_velocity_source

```python
def get_velocity_source(self) -> str:
```

获得机器人的速度命令来源。

### 返回值:
    str: 当前速度命令来源的名称。可能的值为:
        0 - 手柄
        1 - 手持设备
        2 - 导航



## get_stand_pose

```python
def get_stand_pose(self) -> list
```

获得机器人的站立姿态。

### 返回值:
    list[float]: 站姿列表，格式为[delta_z, delta_pitch, delta_yaw, stable_level]

## get_group_state

```python
def get_group_state(self, group_name: str, key: str = 'position') -> list[float]
```

获得机器人特定控制组的状态。


### 参数:
    group_name (str): 机器人控制组的名称。
    key (str): 要从控制组状态中检索的键。有效值:
        'position' - 关节位置
        'velocity' - 关节速度
        'effort' - 关节力矩

### 返回值:
    list[float]: 特定控制组的状态。


## get_cartesian_state

```python
def get_cartesian_state(self, group_name: str, key: str = 'pose') -> list[float]
```

获得机器人特定控制组的笛卡尔状态。

### 参数:
    group_name (str): 机器人控制组的名称。
    key (str): 要从笛卡尔状态中检索的键。有效值:
        'pose' - 控制组姿态
        'twist' - 控制组速度
        'wrench' - 控制组扭矩

### 返回值:
    list[float]: 特定控制组的笛卡尔状态。


## get_base_data

```python
def get_base_data(self, key: str) -> list[float]
```

获得机器人的基座数据。


### 参数:
    key (str): 要从基座数据中检索的键。有效值:
        'quat_xyzw' - 四元数(x, y, z, w)
        'quat_wxyz' - 四元数(w, x, y, z)
        'rpy' - 欧拉角(滚转, 俯仰, 偏航)
        'omega_W' - 角速度(世界坐标系)
        'acc_W' - 线加速度(世界坐标系)
        'omega_B' - 角速度(基座坐标系)
        'acc_B' - 线加速度(基座坐标系)
        'vel_W' - 线速度(世界坐标系)
        'pos_W' - 位置(世界坐标系)
        'vel_B' - 线速度(基座坐标系)
  

### 返回值:
    list[float]: 指定的基座数据。


## get_true_data

```python
def get_true_data(self, key: str) -> list[float]
```

获得机器人的真实数据。

### 参数:
    key (str): 要从真实数据中检索的键。有效值:
        'vel_B' - 线速度(基座坐标系)
        'vel_W' - 线速度(世界坐标系)
        'pos_W' - 位置(世界坐标系)
        'contact_fz' - 接触力和力矩
        'contact_prob' - 接触概率
### 返回值:
    list[float]: 指定的真实数据。


## get_contact_data

```python
def get_true_data(self, key: str) -> list[float]
```

获取机器人的接触数据。

### 参数:
    key (str): 要从接触数据中检索的键。有效值:
        'contact_fz' - 接触力和力矩
        'contact_prob' - 接触概率

### 返回值:
    list[float]: 指定的接触数据。



# Setter Function

## set_fsm_state

```python
def set_fsm_state(self, state: int)
```

设置机器人的状态。

### 参数:
    state (int): 要设置的状态。有效值:  
        0 - 默认状态
        1 - 关节站立
        2 - PD站立
        3~8 - 用户控制器A~F
        9 - 安全保护
        10 - 用户命令
        11 - 上半身用户命令


## set_upper_fsm_state

```python
def set_upper_fsm_state(self, state: int)
```

设置机器人的上半身状态。

### 参数:
    state (int): 要设置的上半身状态。有效值:  
        0 - 默认状态
        1 - 摆臂
        2 - 远程控制


## set_velocity_source

```python
def set_velocity_source(self, source: int)
```
设置机器人的速度命令来源。

### 参数:
    source (int): 要设置的速度命令来源。有效值:  
        0 - 手柄
        1 - 手持设备
        2 - 导航

### 注意:
    对于客户端速度控制，请将速度命令来源设置为2（导航）。


## set_velocity

```python
def set_velocity(self, vx: float, vy: float, yaw: float)
```
设置机器人的速度命令。

### 参数:
    vx (float): 线速度（x方向）（前进/后退）（单位：米/秒）
    vy (float): 线速度（y方向）（左/右）（单位：米/秒）
    yaw (float): 角速度（绕z轴旋转）（单位：弧度/秒）

### 注意:
    实际获得的速度可能受机器人物理能力限制。


## set_stand_pose

```python
def set_stand_pose(self, delta_z: float, delta_pitch: float, delta_yaw: float)
```
设置机器人的站立姿态。

### 参数:
    delta_z (float): ...
    delta_pitch (float): ...
    delta_yaw (float): ...

### 注意:
    仅在PD站立状态（2）下可进行站立姿态调整。实际站立姿态可能受机器人物理能力限制。



## set_joint_positions

```python
def set_joint_positions(self, position_dict: Dict[str, list[float]], is_upper: bool = True)
```
设置机器人的关节位置。


### 参数:
    position_dict (Dict[str, list[float]]): 字典，键为关节名称，值为关节位置。



## set_motor_cfg

```python
def set_motor_cfg(self, kp_config: Dict[str, list[float]], kd_config: Dict[str, list[float]])
```
设置机器人的电机配置。

### 参数:
    kp_config (Dict[str, list[float]]): 字典，键为控制组名称，值为Kp值列表。
    kd_config (Dict[str, list[float]]): 字典，键为控制组名称，值为Kd值列表。

### 注意:

    PD配置字典的键位应该与机器人配置文件中的控制组名称匹配。
