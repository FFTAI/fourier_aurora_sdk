# 开发者指南

本指南面向希望在 Fourier 机器人上使用 Fourier Aurora SDK 进行自定义应用开发的开发者。它提供了如何根据项目需求使用 SDK 的信息。

Aurora 运行一个 FSM 状态机，每个状态对应特定的任务（控制器）。您可以通过摇杆或 DDS 接口发送命令在这些状态之间切换。每个任务都有自己的输入和输出。您可以使用 **摇杆** 或 **DDS** 与任务交互。

各机型的状态和控制器的描述请参考 [GR-1 机器人控制器参考](./gr1/robot_controller_reference_CN.md)、[GR-2 机器人控制器参考](./gr2/robot_controller_reference_CN.md) 和 [GR-3 机器人控制器参考](./gr3/robot_controller_reference_CN.md)。

### 摇杆控制

摇杆为 Aurora 提供有限的输入，包括状态切换、速度控制和站立姿势调整。有关摇杆控制的详细信息，请参阅 [摇杆教程](../../doc/CN/joystick_tutorial_CN.md)。

### DDS 控制

DDS（数据分发服务）是一种通信协议，允许 Aurora 与其他程序或设备通信。DDS 提供了一种灵活高效的方式来改变 Aurora 的状态。它可用于控制 Aurora 的运动，以及获取 Aurora 的信息，如当前状态、关节角度和传感器读数。有关 Aurora 中 DDS 使用的详细信息，请参阅 [Aurora DDS 参考](./aurora_dds_reference_CN.md)。

#### Python DDS 接口

Aurora 提供了一个 Python DDS 客户端，允许您使用 DDS 与 Aurora 发送和接收数据。更多信息请参考 `python` 目录。
