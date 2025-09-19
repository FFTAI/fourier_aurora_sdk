# 开发者指南

本指南面向希望在Fourier机器人上使用Fourier Aurora SDK开发自己应用的开发者。它根据项目需求提供了如何使用SDK的信息。

## 使用预置控制器

Aurora提供了一系列可用于控制机器人运动的控制器。一些通用控制器包含在`fourier-aurora`包中，而其他控制器则根据机器人配置包含在扩展包中，例如GR-2机器人的`fourier-aurora-gr2`包。

Aurora运行一个FSM状态机，每个状态对应特定的任务(控制器)。您可以通过摇杆或DDS接口发送命令在这些状态之间切换。每个任务都有自己的输入和输出。您可以使用**摇杆**或**DDS**与任务交互。

状态和控制器的描述请参考[控制器参考](./robot_controller_reference_CN.md)。

### 摇杆控制

摇杆为Aurora提供有限的输入，包括状态切换、速度控制和站立姿势调整。有关摇杆控制的详细信息，请参阅[摇杆教程](../../doc/CN/joystick_tutorial_CN.md)。

### DDS控制

DDS(数据分发服务)是一种通信协议，允许Aurora与其他程序或设备通信。DDS提供了一种灵活高效的方式来改变Aurora的状态。它可用于控制Aurora的运动，以及获取Aurora的信息，如当前状态、关节角度和传感器读数。

#### Python DDS接口

Aurora提供了一个Python DDS客户端，允许您使用DDS与Aurora发送和接收数据。更多信息请参考`python`目录。

#### C++ DDS接口

对于C++开发者，目前Aurora中没有提供C++ DDS客户端。但是，您可以使用提供的C++ fourierdds接口编写发布者和订阅者来与Aurora交互。更多信息请参考`example/gr2/dds_comm_task`目录。

## 在Aurora中编写自定义控制器

要在Aurora中编写自定义控制器，请参阅[任务教程](../../doc/CN/task_tutorial_CN.md)获取更多信息。
