# Custom DDS IDL Tutorial

The inputs and outputs of the controllers built in Aurora can be transmitted over DDS messages. The DDS IDL file is used to define the data types and message formats used for communication between the controllers and the DDS middleware. There are several pre-defined DDS IDL files in Aurora, but if you need to transmit data types that are not included in these files, you can create your own custom DDS IDL file.

This tutorial will guide you through the process of creating a custom DDS IDL file and how to use it in Aurora.

## Creating a Custom DDS IDL File and Installing it

1. Create a new file with a `.idl` extension under the `idl` directory. For example, `TeleoperationCmd.idl`.

2. Define the data types and message formats in the IDL file. For example, the following code defines a message format for sending a teleoperation command:

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

    The `Time`, `GroupCmd`, and `TeleoperationCmd` are the data types defined in the IDL file. They are included in the `msg` sub-module of the `fourier_msgs` module.

3. Install the custom DDS IDL file in Aurora.

    To do this, simply run the script `generate_and_install_msg.sh` in the root directory. This script will generate the necessary code for the custom DDS IDL file and install it in the Aurora installation directory.

    ```
    bash generate_and_install_msg.sh
    ```

## Using the Custom DDS IDL File in Aurora

1. Necessary includes and CMake configuration:

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

2. Create a DDS node and publishers or subscribers:

    ```c++
    // Create the DDS node. The first argument is the DDS domain ID and the second argument is the default data mode. It is a must that the domain ID for two nodes is the same to enable communication between them.

    auto dds_node = std::make_shared<fourier_dds::DdsNode>(1, 0);

    // Publisher arguments: 
    // 1. The DDS node
    // 2. The topic name
    // 3. Whether to wait for the subscriber to be available before publishing, recommended to set to false
    // 4. wait time, recommended to set to 0
    auto publisher = std::make_shared<fourier_dds::DdsPublisher<fourier_msgs::msg::TeleoperationCmdPubSubType>>(
        dds_node, "teleoperation_cmd", false, 0);
    
    fourier_msgs::msg::TeleoperationCmd teleoperation_cmd;
    teleoperation_cmd.stamp().sec(123);
    teleoperation_cmd.stamp().nanosec(123456789);
    teleoperation_cmd.groups_cmd().resize(1);
    teleoperation_cmd.groups_cmd().at(0).group_name("group1");
    teleoperation_cmd.groups_cmd().at(0).position({0.1, 0.2, 0.3});

    // Publish the message
    publisher->publish(teleoperation_cmd);
    

    // Define the callback function
    auto callback = [](const fourier_msgs::msg::TeleoperationCmd &msg) {
        std::cout << "Received teleoperation command: " << msg.stamp().sec() << "s " << msg.stamp().nanosec() << "ns" << std::endl;
        std::cout << "Group name: " << msg.groups_cmd().at(0).group_name() << std::endl;
        for (const auto& pos : msg.groups_cmd().at(0).position()) {
            std::cout << "Position: " << pos << std::endl;}
    };
    // Subscriber arguments: 
    // 1. The DDS node
    // 2. The topic name
    // 3. The callback function
    auto subscription = std::make_shared<fourier_dds::DdsSubscription<fourier_msgs::msg::TeleoperationCmdPubSubType>>(
        dds_node, "teleoperation_cmd", callback);
    ```

    In this example, we create a publisher and a subscriber example for the `TeleoperationCmd` message. The publisher publishes a `TeleoperationCmd` message with a single control group command. The subscriber subscribes to the same topic and prints the received message to the console.

3. Compile and run the code.
