#pragma once

#include "state/Task.hpp"
#include "robot_data/RobotData.hpp" 
#include "TeleoperationCmd/TeleoperationCmdPubSubTypes.hpp"
#include "fourier_dds/dds_node.hpp"
#include "fourier_dds/dds_publisher.hpp"
#include "fourier_dds/dds_subscription.hpp"


class DdsCommTask : public TaskInterface {
public:
    DdsCommTask(RobotData& data);
    ~DdsCommTask();
public:
    void execute(RobotData& data) override;
    bool enter(RobotData& data) override;
    bool exit(RobotData& data) override;
    bool init(RobotData& data) override;
    std::string getTaskName() const override {
        return "DdsCommTask";
    }

private:
    bool isRunning = false;
    double dt;
    double runTime;
    std::shared_ptr<fourier_dds::DdsNode> dds_node;
    std::shared_ptr<fourier_dds::DdsPublisher<fourier_msgs::msg::TeleoperationCmdPubSubType>> publisher;
    std::shared_ptr<fourier_dds::DdsSubscription<fourier_msgs::msg::TeleoperationCmdPubSubType>> subscription;
};

extern "C" {
    TaskInterface* createTask(RobotData& data);  // 工厂函数，返回对应任务的实例
}


