# Task Tutorial

This tutorial will guide you through the process of creating a new FSM task and running it in Aurora. After completing this tutorial, you can try to build your own controllers within Aurora. For better understanding, we recommend that you finish [Introduction](introduction_EN.md) before proceeding with this tutorial. Besides, [Shared Data Reference](robotdata_reference_EN.md) and [Custom DDS IDL Tutorial](custom_dds_idl_tutorial_EN.md) are also recommended to read.

## Compiling and Installing the Example Task

Please follow the steps below to compile and run the example task provided in the docker container:

1. bash generate_and_install_msg.sh
2. cd example/gr2
3. mkdir build && cd build
4. cmake ..
5. make
6. make install

This will compile and install the example task as a dynamic library in the container.

## Template Task

Please take the `movejoint_task` folder as a reference for creating a new task.

In `init` and `enter` functions, you can define the initial state and any other necessary variables. They will be called when the task is initialized and when it enters the running state.

In `execute` function, you can define the behavior of the task. This function is called periodically until the task is terminated.

In `exit` function, you can define any cleanup operations that need to be performed when the task is terminated.

## Running the Task

To run the task in Aurora, follow these steps:

1. Add the task in the `user runner config.yaml` file under the `gr2/config` folder

    ```yaml
    UserController_B: 
        allow_upper_body_override: false
        MoveJointTask:
            period: 0.1
            cup_priority: 1
    ```

    In this example, we have added the `MoveJointTask` to the `UserController_B` State. The `period` parameter specifies the time interval(in seconds) between successive calls to the `execute` function. `allow_upper_body_override` parameter specifies a flag value, indicating whether other tasks can override the upper body motion.

    Note that the task name `MoveJointTask` **must** match the name of the shared library file. Plese check the CMakeLists.txt file in the task folder to confirm the name of the shared library file.

    ```CMake
    add_library(runner_task_MoveJointTask SHARED
        ${MOVEJOINT_TASK_SOURCE_FILES}
    )
    ```

    In this example, the shared library file name is `runner_task_MoveJointTask`.

2. Build the task, install the shared library and config files

    Please make sure that the task folder is added in the root CMakeLists.txt file.

    ```CMake
    add_subdirectory(movejoint_task)
    ```

    Then, build the tasks in the `build` folder under `gr2` directory using the following command:

    ```bash
    cd gr2
    mkdir build (if not already created)
    cd build
    cmake ..
    make
    make install
    ```

    You can check if the shared library is successfully installed in the `/usr/local/Aurora/lib/user_task_lib` directory.

3. Start Aurora and switch to the `UserController_B` state

    Before starting the Aurora program, please make sure that the actuators are all in Power-on state(purple light flashing slowly) and network communication is well established(all actuators ip pingable). Then you can start the Aurora program by running the following command in your terminal:

    ```bash
    AuroraCore --config config/config.yaml
    ```

    The following message will appear in the terminal if the program is running successfully:

    ```bash
    [info] FSM state run, enter "Default" mode by default
    ```

    This message indicates that the self-check is passed, all the actuators are enabled and the Aurora program is ready to run tasks.

    After the Aurora program is running, switch to the `UserController_B` state by pressing the *RB_B* button on the joystick. The following message will appear in the terminal if the task is running successfully:

    ```bash
    Enter Movejoint Task ...
    ```

    This message indicates that the `MoveJointTask` is running successfully. You can see the task's behavior in the terminal.
