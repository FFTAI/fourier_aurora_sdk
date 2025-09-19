/**
 * @file SimpleLocoTask.hpp
 * @brief Header file for SimpleLocoTask in robot controllers.
 *
 * @details 
 *
 * @author renminwansui
 * @date April 30, 2025
 * @version v1.0
 * @bug No known bugs
 *
 * @copyright Copyright (c) 2025, shanghai fourier intelligence
 */


#pragma once

#include "state/Task.hpp"
#include "robot_data/RobotData.hpp"
#include "PolicyParent.hpp"
#include "SimpleLocoParameter.hpp"
#include "SimpleLocoPolicy.hpp"

enum class SimpleLocoTaskState {
    INIT,
    ENTER,
    RUNNING,
    EXIT,
    ERROR
};

class SimpleLocoTask : public TaskInterface {
public:
    SimpleLocoTask(RobotData& data);
    ~SimpleLocoTask();
public:
    void execute(RobotData& data) override;
    bool enter(RobotData& data) override;
    bool exit(RobotData& data) override;
    bool init(RobotData& data) override;
    std::string getTaskName() const override {
        return "SimpleLocoTask";
    }

private:
    SimpleLocoTaskState task_state_;
    double dt_;

    double control_freq_;

    int robot_joint_num_;
    int control_joint_num_;

    Eigen::VectorXd default_pos_;
    Eigen::VectorXd q_kp_;
    Eigen::VectorXd q_kd_;

    std::unique_ptr<SimpleLocoPolicy> simple_loco_policy_;

    Eigen::VectorXd robot_q_cmd_;
    
    bool isRunning_ = false;
    ParameterManager& config_ = ParameterManager::get_instance();
};

extern "C" {
    TaskInterface* createTask(RobotData& data);  // 工厂函数，返回对应任务的实例
}