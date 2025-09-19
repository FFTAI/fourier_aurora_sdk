# 自定义 DDS IDL 教程

在 Aurora 中构建的控制器的输入和输出可以通过 DDS 消息进行传输。DDS IDL 文件是定义控制器与 DDS 中间件之间通信所用的数据类型和消息格式的关键。它允许您精确指定消息的结构、字段类型和模块组织方式，确保通信双方对数据格式有统一的理解。

Aurora 已经内置了若干 DDS IDL 文件，涵盖了常见的控制命令和状态反馈需求。但如果您需要传输这些文件未包含的特殊数据类型，或者需要定制化的消息结构，您可以创建自己的自定义 DDS IDL 文件。这在以下场景特别有用：
- 需要添加新的控制命令类型
- 需要扩展现有的消息结构
- 需要优化通信效率而调整数据类型

本教程将详细指导您完成创建自定义 DDS IDL 文件的完整流程，并展示如何在 Aurora 中有效地使用这些自定义消息。

## 创建自定义 DDS IDL 文件并安装

1. 在 `idl` 目录下新建一个 `.idl` 后缀的文件。例如：`TeleoperationCmd.idl`。

2. 在 IDL 文件中定义数据类型和消息格式。例如，以下代码定义了一个用于发送遥操作指令的消息格式：

    ```idl
    module fourier_msgs { module msg { 
    /** \brief time stamp message */
    struct Time{
        int32 sec;
        uint32 nanosec;
    };
    /** \brief group control cmd message */
    struct GroupCmd {
        string group_name;
        sequence<double> position; 
    };

    /** \brief teleoperation cmd message */
    struct TeleoperationCmd {
        Time stamp;
        sequence<GroupCmd> groups_cmd;
    };
    }; };

    ```

    `Time`、`GroupCmd` 和 `TeleoperationCmd` 是在 IDL 文件中定义的数据类型，均包含在 `fourier_msgs` 模块的 `msg` 子模块下。

3. 在 Aurora 中安装自定义 DDS IDL 文件。

    只需在根目录下运行 `generate_and_install_msg.sh` 脚本即可。该脚本会为自定义 DDS IDL 文件生成所需代码，并安装到 Aurora 安装目录。

    ```
    bash generate_and_install_msg.sh
    ```

## 在 Aurora 中使用自定义 DDS IDL 文件

1. 必要的头文件和 CMake 配置：

    ```c++
    #include "fourier_dds/dds_node.hpp"
    #include "fourier_dds/dds_publisher.hpp" 
    #include "fourier_dds/dds_subscription.hpp"
    #include "TeleoperationCmd/TeleoperationCmdPubSubTypes.hpp"
    ```

    ```CMake
    find_package(fourier_dds REQUIRED)

    target_include_directories(runner_task_xxxTask 
    PUBLIC 
    ${fourier_dds_INCLUDE_DIRS}
    )

    target_link_directories(runner_task_xxxTask 
    PUBLIC 
    ${fourier_dds_LIBRARY_DIRS}
    )

    file(GLOB FOURIER_DDS_LIBS "${fourier_dds_LIBRARY_DIRS}/*.so")

    target_link_libraries(runner_task_xxxTask 
    PUBLIC 
    ${FOURIER_DDS_LIBS}
    )
    ```

2. 创建 DDS 节点以及发布者或订阅者：

    ```c++
    // 创建 DDS 节点。第一个参数为 DDS 域 ID，第二个参数为默认数据模式。要实现节点间通信，两个节点的域 ID 必须一致。

    auto dds_node = std::make_shared<fourier_dds::DdsNode>(1, 0);

    // 发布者参数详细说明：
    // 1. DDS 节点 - 必须与订阅方使用相同的节点实例才能通信
    // 2. 话题名称 - 必须与订阅方指定的名称完全匹配(区分大小写)
    // 3. 是否等待订阅者 - 设为false可立即发布，设为true会阻塞直到有订阅者连接
    // 4. 等待时间 - 当参数3为true时生效，0表示无限等待
    auto publisher = std::make_shared<fourier_dds::DdsPublisher<fourier_msgs::msg::TeleoperationCmdPubSubType>>(
        dds_node, "teleoperation_cmd", false, 0);
    
    fourier_msgs::msg::TeleoperationCmd teleoperation_cmd;
    teleoperation_cmd.stamp().sec(123);
    teleoperation_cmd.stamp().nanosec(123456789);
    teleoperation_cmd.groups_cmd().resize(1);
    teleoperation_cmd.groups_cmd().at(0).group_name("group1");
    teleoperation_cmd.groups_cmd().at(0).position({0.1, 0.2, 0.3});

    // 发布消息
    publisher->publish(teleoperation_cmd);
    

    // 定义回调函数 - 当订阅者收到新消息时会自动调用此函数
    // 回调函数应该尽可能高效，避免长时间阻塞
    // 如果需要复杂处理，建议将消息放入队列由其他线程处理
    auto callback = [](const fourier_msgs::msg::TeleoperationCmd &msg) {
        std::cout << "Received teleoperation command: " << msg.stamp().sec() << "s " << msg.stamp().nanosec() << "ns" << std::endl;
        std::cout << "Group name: " << msg.groups_cmd().at(0).group_name() << std::endl;
        for (const auto& pos : msg.groups_cmd().at(0).position()) {
            std::cout << "Position: " << pos << std::endl;}
    };
    // 订阅者参数：
    // 1. DDS 节点
    // 2. 话题名称
    // 3. 回调函数
    auto subscription = std::make_shared<fourier_dds::DdsSubscription<fourier_msgs::msg::TeleoperationCmdPubSubType>>(
        dds_node, "teleoperation_cmd", callback);
    ```

    在本例中，我们为 `TeleoperationCmd` 消息创建了一个发布者和一个订阅者示例。发布者发布包含单个控制组指令的 `TeleoperationCmd` 消息，订阅者订阅同一话题并将收到的消息打印到控制台。

3. 编译并运行代码。
