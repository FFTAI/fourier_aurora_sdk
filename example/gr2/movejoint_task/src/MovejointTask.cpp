#include "MovejointTask.hpp"

extern "C" TaskInterface* createTask(RobotData& data) {
    return new MovejointTask(data);  // 返回具体任务类的实例
}

MovejointTask::MovejointTask(RobotData& data): TaskInterface(data)
{   
    dt = 0.1;
}
MovejointTask::~MovejointTask()
{

}
void MovejointTask::execute(RobotData& data)
{
    grx_sot::robot::RobotState robot_state_fixed = data.hardwareData.getRobotStateFixed(); // get the current robot state from hardwareData
    Eigen::VectorXd q = robot_state_fixed.q;
    std::cout << "q: " << q.transpose() << std::endl;
    data.hardwareData.setWholeBodyPosCmd(q); // send control command using hardwareData 

    runTime += dt;
}

bool MovejointTask::enter(RobotData& data){
    std::cout <<"               Enter Movejoint Task ...\n";
    runTime = 0;
    isRunning = true;
    return isRunning;
}
bool MovejointTask::exit(RobotData& data){
    std::cout <<"           Exit Movejoint Task ...\n";
    std::this_thread::sleep_for(std::chrono::milliseconds(10));       
    isRunning = false;
    return !isRunning;
}
bool MovejointTask::init(RobotData& data){
    return true;
}