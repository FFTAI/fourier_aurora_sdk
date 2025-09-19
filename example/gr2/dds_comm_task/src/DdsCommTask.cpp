#include "DdsCommTask.hpp"

extern "C" TaskInterface* createTask(RobotData& data) {
    return new DdsCommTask(data);  // 返回具体任务类的实例
}

DdsCommTask::DdsCommTask(RobotData& data): TaskInterface(data)
{   
    dt = 0.1;
    dds_node = std::make_shared<fourier_dds::DdsNode>(1, 0);
    publisher = std::make_shared<fourier_dds::DdsPublisher<fourier_msgs::msg::TeleoperationCmdPubSubType>>(
        dds_node, "teleoperation_cmd", false, 0);
    auto callback = [](const fourier_msgs::msg::TeleoperationCmd &msg) {
        std::cout << "Received teleoperation command: " << msg.stamp().sec() << "s " << msg.stamp().nanosec() << "ns" << std::endl;
        std::cout << "Group name: " << msg.groups_cmd().at(0).group_name() << std::endl;
        for (const auto& pos : msg.groups_cmd().at(0).position()) {
            std::cout << "Position: " << pos << std::endl;}
    };
    
    subscription = std::make_shared<fourier_dds::DdsSubscription<fourier_msgs::msg::TeleoperationCmdPubSubType>>(
        dds_node, "teleoperation_cmd", callback);
        
}
DdsCommTask::~DdsCommTask()
{

}
void DdsCommTask::execute(RobotData& data)
{
    fourier_msgs::msg::TeleoperationCmd teleoperation_cmd;
    teleoperation_cmd.stamp().sec(123);
    teleoperation_cmd.stamp().nanosec(123456789);
    teleoperation_cmd.groups_cmd().resize(1);
    teleoperation_cmd.groups_cmd().at(0).group_name("group1");
    teleoperation_cmd.groups_cmd().at(0).position({0.1, 0.2, 0.3});

    // Publish the message
    publisher->publish(teleoperation_cmd);
    runTime += dt;
}

bool DdsCommTask::enter(RobotData& data){
    std::cout <<"               Enter DdsComm Task ...\n";
    runTime = 0;
    isRunning = true;
    return isRunning;
}
bool DdsCommTask::exit(RobotData& data){
    std::cout <<"           Exit DdsComm Task ...\n";
    std::this_thread::sleep_for(std::chrono::milliseconds(10));       
    isRunning = false;
    return !isRunning;
}
bool DdsCommTask::init(RobotData& data){
    return true;
}