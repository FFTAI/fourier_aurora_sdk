# 客户端使用教程

本教程将以client_usage.py为例，介绍如何创建Aurora客户端和Aurora本体的DDS通讯，并通过DDS通讯与Aurora本体进行数据交换。

## 1. 导入依赖库

首先，导入依赖库。

```python
from fourier_aurora_client import AuroraClient

```

## 2. 实例化Aurora客户端

```python
client = AuroraClient.get_instance(domain_id=160, participant_qos = 0,robot_name="gr2t2v2", serial_number=None)
```
实例化Aurora客户端，需要传入四个参数：

- domain_id: DDS域ID，默认为123
- participant_qos: QoS设置，默认为0。
- robot_name: 机器人名称，默认为"gr2t2v2"。
- serial_number: 机器人序列号，默认为None。

注意：以上参数需要与Aurora的DDS设置一致。

## 3. 数据预处理
```python
positions = {
    "left_leg": interpolate_position(init_pos["left_leg"], target_pos["left_leg"], step, total_steps),
    "right_leg": interpolate_position(init_pos["right_leg"], target_pos["right_leg"], step, total_steps),
    "waist": interpolate_position(init_pos["waist"], target_pos["waist"], step, total_steps),
    "head": interpolate_position(init_pos["head"], target_pos["head"], step, total_steps),
    "left_manipulator": interpolate_position(init_pos["left_manipulator"], target_pos["left_manipulator"], step, total_steps),
    "right_manipulator": interpolate_position(init_pos["right_manipulator"], target_pos["right_manipulator"], step, total_steps),
    "left_hand": interpolate_position(init_pos["left_hand"], target_pos["left_hand"], step, total_steps),
    "right_hand": interpolate_position(init_pos["right_hand"], target_pos["right_hand"], step, total_steps),
}

```
本教程的目的是要控制机器人平滑地运动，因此需要对机器人基于初始关节位置和目标关节位置进行插值，并将关节位置指令转换成字典的形式，
字典的key就是控制组的名字，value就是列表形式的控制组的指令。



## 4. 发送指令

```python
input("switch to PD stand task\nPlease desired state number: ")

client.set_fsm_state(2)
```
发送切换到PD stand task的指令。

```python
input("set robot joints to desired position\nPress Enter to continue...")
move_joints(client, INIT_POS, TARGET_POS, FREQUENCY, DURATION)

# Move back to initial position
input("set robot joints to home position\nPress Enter to continue...")
move_joints(client, TARGET_POS, INIT_POS, FREQUENCY, DURATION)
```

这里调用了自定义的`move_joints`函数，控制机器人从初始位置平滑移动到目标位置，并在移动完成后返回到初始位置。move_joints函数里输入了初始位置，目标位置，频率和持续时间，计算出插值后的目标关节位置，调用客户端的`set_joint_positions`函数发送关节位置指令。

```python
client.set_joint_positions(positions)
```

## 5. 关闭客户端

```python
client.close()
```
发送完指令后，关闭客户端。



