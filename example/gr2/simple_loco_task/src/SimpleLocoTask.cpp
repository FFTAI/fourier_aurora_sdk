#include "SimpleLocoTask.hpp"
#include "SimpleLocoPolicy.hpp"

extern "C" TaskInterface* createTask(RobotData& data) {
    return new SimpleLocoTask(data);  // 返回具体任务类的实例
}

SimpleLocoTask::SimpleLocoTask(RobotData& data): TaskInterface(data)
{   
    config_.addParameter<SimpleLocoParameter>("simple_loco_parameter", config_.env_info.config_file_path);
    auto* simple_loco_parameter = dynamic_cast<SimpleLocoParameter*>(config_.getTaskParameter("simple_loco_parameter"));

    dt_ = simple_loco_parameter->dt;

    control_freq_ = simple_loco_parameter->control_freq;

    robot_joint_num_ = simple_loco_parameter->robot_joint_num;
    control_joint_num_ = simple_loco_parameter->control_joint_num;

    robot_q_cmd_ = Eigen::VectorXd::Zero(robot_joint_num_);

    default_pos_ = simple_loco_parameter->default_pos;

    q_kp_ = simple_loco_parameter->kp_joint_space;
    q_kd_ = simple_loco_parameter->kd_joint_space;

    simple_loco_policy_ = std::make_unique<SimpleLocoPolicy>(simple_loco_parameter);
    
    task_state_ = SimpleLocoTaskState::INIT;
}

SimpleLocoTask::~SimpleLocoTask()
{

}

bool SimpleLocoTask::init(RobotData& data){
    std::cout <<"           Init Simple Loco Task ..." << std::endl;
    std::cout <<"           Init Simple Loco Task Done" << std::endl;
    return true;
}


bool SimpleLocoTask::enter(RobotData& data){
    std::cout <<"               Enter Simple Loco Task ..." << std::endl;
    task_state_ = SimpleLocoTaskState::INIT;

    simple_loco_policy_->init(data);
    task_state_ = SimpleLocoTaskState::ENTER;

    for(int i = 0; i < 2; i++) {
        data.hardwareData.setWholeBodyPDModePD(q_kp_, q_kd_);
        std::this_thread::sleep_for(std::chrono::milliseconds(10)); 
    }

    simple_loco_policy_->enter(data);
    isRunning_ = true;
    task_state_ = SimpleLocoTaskState::RUNNING;
    std::cout <<"               Enter Simple Loco Task Done" << std::endl;
    return isRunning_;
}


void SimpleLocoTask::execute(RobotData& data)
{
    simple_loco_policy_->runPolicy(data);
    Eigen::VectorXd action_command = simple_loco_policy_->getActionCommand();

    robot_q_cmd_.head(control_joint_num_) = action_command;

    data.hardwareData.setWholeBodyPosCmd(robot_q_cmd_);
}



bool SimpleLocoTask::exit(RobotData& data){
    std::cout <<"           Exit Simple Loco Task ..." << std::endl;
    simple_loco_policy_->exit(data);
    std::this_thread::sleep_for(std::chrono::milliseconds(10));

    isRunning_ = false;
    task_state_ = SimpleLocoTaskState::EXIT;
    std::cout <<"           Exit Simple Loco Task Done" << std::endl;
    return !isRunning_;
}


