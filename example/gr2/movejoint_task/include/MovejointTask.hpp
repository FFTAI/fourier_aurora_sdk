#pragma once

#include "state/Task.hpp"
#include "robot_data/RobotData.hpp" 

class MovejointTask : public TaskInterface {
public:
    MovejointTask(RobotData& data);
    ~MovejointTask();
public:
    void execute(RobotData& data) override;
    bool enter(RobotData& data) override;
    bool exit(RobotData& data) override;
    bool init(RobotData& data) override;
    std::string getTaskName() const override {
        return "MovejointTask";
    }

private:
    bool isRunning = false;
    double dt;
    double runTime;
};

extern "C" {
    TaskInterface* createTask(RobotData& data);  // 工厂函数，返回对应任务的实例
}


