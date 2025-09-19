# 任务教程

本教程将指导你如何在 Aurora 中创建一个新的 FSM 任务并运行它。完成本教程后，你可以尝试在 Aurora 内部构建自己的控制器。为了更好地理解，建议你先阅读[简介](./introduction.md)后再进行本教程。

## 模板任务

请参考 `movejoint_task` 文件夹来创建新任务。

在 `init` 和 `enter` 函数中，你可以定义初始状态和其他必要的变量。这些函数会在任务初始化和进入运行状态时被调用。

在 `execute` 函数中，你可以定义任务的具体行为。该函数会被周期性调用，直到任务终止。

在 `exit` 函数中，你可以定义任务终止时需要执行的清理操作。

## 运行任务

在 Aurora 中运行任务，请按照以下步骤操作：

1. 在 `gr2/config` 文件夹下的 `user runner config.yaml` 文件中添加任务

    ```yaml
    UserController_B: 
        allow_upper_body_override: false
        MoveJointTask:
            period: 0.1
            cup_priority: 1
    ```

    在本例中，我们将 `MoveJointTask` 添加到了 `UserController_B` 状态。`period` 参数指定了连续两次调用 `execute` 函数的时间间隔（单位：秒）。`allow_upper_body_override` 参数指定是否允许其他任务覆盖上半身动作。

    注意，任务名 `MoveJointTask` **必须**与共享库文件名一致。请检查任务文件夹下的 CMakeLists.txt 文件以确认共享库文件名。

    ```CMake
    add_library(runner_task_MoveJointTask SHARED
        ${MOVEJOINT_TASK_SOURCE_FILES}
    )
    ```

    在本例中，共享库文件名为 `runner_task_MoveJointTask`。

2. 编译任务，安装共享库和配置文件

    请确保任务文件夹已在根 CMakeLists.txt 文件中添加。

    ```CMake
    add_subdirectory(movejoint_task)
    ```

    然后，在 `gr2` 目录下的 `build` 文件夹中编译任务，命令如下：

    ```bash
    cd gr2
    mkdir build （如未创建）
    cd build
    cmake ..
    make
    make install
    ```

    你可以在 `/usr/local/Aurora/lib/user_task_lib` 目录下检查共享库是否已成功安装。

3. 启动 Aurora 并切换到 `UserController_B` 状态

    启动 Aurora 程序前，请确保所有执行器均处于上电状态（紫灯慢闪），且网络通信正常（所有执行器 IP 可 ping 通）。然后在终端运行以下命令启动 Aurora 程序：

    ```bash
    AuroraCore
    ```

    若程序运行成功，终端会出现如下信息：

    ```bash
    [info] FSM state run, enter "Default" mode by default
    ```

    该信息表示自检通过，所有执行器已使能，Aurora 程序已准备好运行任务。

    Aurora 程序运行后，按下手柄上的 *RB_B* 按钮切换到 `UserController_B` 状态。如果任务运行成功，终端会出现如下信息：

    ```bash
    Enter Movejoint Task ...
    ```

    该信息表示 `MoveJointTask` 已成功运行。你可以在终端看到任务的相关行为输出。
