#include "SimpleLocoPolicy.hpp"

void SimpleLocoPolicy::init(RobotData& data){
    std::cout <<"           Init Simple Loco Policy Start" << std::endl;
    dt_ = simple_loco_parameter_->dt;
    control_freq_ = simple_loco_parameter_->control_freq;

    gait_generator_ = std::make_unique<GaitGenerator>(simple_loco_parameter_);
    gait_generator_->init(data);
    stand_hold_time_ = 0;

    num_actor_obs_ = simple_loco_parameter_->num_actor_obs;
    num_actor_hist_ = simple_loco_parameter_->num_actor_hist;
    num_action_joint_control_ = simple_loco_parameter_->control_joint_num;
    std::cout<<"num_action_joint_control_: "<<num_action_joint_control_<<std::endl;

    device_ = new torch::Device(torch::kCPU);
    policy_ = torch::jit::load(simple_loco_parameter_->policy_path);
    policy_.to(*device_);

    control_joint_default_pos_ = simple_loco_parameter_->default_pos.head(num_action_joint_control_);
    control_joint_kp_ = simple_loco_parameter_->kp_joint_space.head(num_action_joint_control_);
    control_joint_kd_ = simple_loco_parameter_->kd_joint_space.head(num_action_joint_control_);
    std::cout << "control_joint_default_pos_: " << control_joint_default_pos_.transpose() << std::endl;
    std::cout << "control_joint_kp_: " << control_joint_kp_.transpose() << std::endl;
    std::cout << "control_joint_kd_: " << control_joint_kd_.transpose() << std::endl;

    obs_scales_lin_vel_ = simple_loco_parameter_->obs_scales_lin_vel;
    obs_scales_ang_vel_ = simple_loco_parameter_->obs_scales_ang_vel;

    obs_scales_command_.resize(3);
    obs_scales_command_ << obs_scales_lin_vel_, obs_scales_lin_vel_, obs_scales_ang_vel_;

    obs_scale_gravity_ = simple_loco_parameter_->obs_scale_gravity;
    obs_scale_joint_pos_ = simple_loco_parameter_->obs_scale_joint_pos;
    obs_scale_joint_vel_ = simple_loco_parameter_->obs_scale_joint_vel;
    obs_scale_height_measurements_ = simple_loco_parameter_->obs_scale_height_measurements;

    obs_scale_action_ = simple_loco_parameter_->obs_scale_action;

    act_scale_action_.resize(num_action_joint_control_);
    act_scale_action_ = simple_loco_parameter_->act_scale_action;

    robot_motion_command_ = Eigen::VectorXd::Zero(3);

    command_filter_cf_ = simple_loco_parameter_->command_filter_cf;
    command_filter_tau_ = 1.0 / (command_filter_cf_ * 2 * M_PI);
    command_filter_alpha_ = command_filter_tau_ / (command_filter_tau_ + (1.0 / control_freq_));
    command_safety_ratio_ = simple_loco_parameter_->command_safety_ratio;

    command_vel_x_range_ = simple_loco_parameter_->command_vel_x_range * command_safety_ratio_;
    command_vel_y_range_ = simple_loco_parameter_->command_vel_y_range * command_safety_ratio_;
    command_vel_yaw_range_ = simple_loco_parameter_->command_vel_yaw_range * command_safety_ratio_;
    

    joint_pos_meas_ = Eigen::VectorXd::Zero(num_action_joint_control_);
    joint_vel_meas_ = Eigen::VectorXd::Zero(num_action_joint_control_);
    joint_trq_meas_ = Eigen::VectorXd::Zero(num_action_joint_control_);
    joint_pos_meas_def_diff_ = Eigen::VectorXd::Zero(num_action_joint_control_);

    base_ori_quat_to_world_ = Eigen::Quaterniond::Identity();

    // buffer
    base_xyz_vel_to_self_ = Eigen::VectorXd::Zero(3);
    base_rpy_vel_to_self_ = Eigen::VectorXd::Zero(3);
    base_xyz_vel_to_world_ = Eigen::VectorXd::Zero(3);
    base_quat_to_world_ = Eigen::Quaterniond::Identity();
    base_project_gravity_ = Eigen::VectorXd::Zero(3);

    // 目标关节位置偏移
    target_joint_position_offset_ = Eigen::VectorXd::Zero(num_action_joint_control_);
    action_target_joint_position_ = Eigen::VectorXd::Zero(num_action_joint_control_);

    simple_loco_state_ = SimpleLocoState::IDLE;

    initActor();
    std::cout <<"           Init Simple Loco Policy Done" << std::endl;
}

void SimpleLocoPolicy::enter(RobotData& data){
    std::cout <<"           Enter Simple Loco Policy Start" << std::endl;
    simple_loco_state_ = SimpleLocoState::IDLE;

    torch::Tensor act_in_tensor_init = torch::tensor(act_in_).to(*device_);
    policy_.forward({act_in_tensor_init}).toTensor().to(*device_);
    
    updateBuffer(data);
    runActor();
    
    std::cout <<"           Enter Simple Loco Policy Done" << std::endl;
}

void SimpleLocoPolicy::runPolicy(RobotData& data)
{
    robot_motion_command_ = data.vel_cmd;
    
    gait_generator_->step();

    updateSimpleLocoState();
    if(simple_loco_state_ == SimpleLocoState::STAND){
        robot_motion_command_.setZero(3);
    }

    updateBuffer(data);

    runActor();

}

void SimpleLocoPolicy::exit(RobotData& data){

    simple_loco_state_ = SimpleLocoState::IDLE;
    gait_generator_->setGaitPattern(GaitPatterns::GAIT_PATTERN_OFF);
}


void SimpleLocoPolicy::initActor(){
    std::cout << "initActor" << std::endl;
    act_in_cmds_ = Eigen::VectorXd::Zero(3);
    act_in_base_ang_vel_ = Eigen::VectorXd::Zero(3);
    act_in_base_proj_grav_ = Eigen::VectorXd::Zero(3);
    act_in_meas_q_pos_def_diff_ = Eigen::VectorXd::Zero(num_action_joint_control_);
    act_in_meas_q_vel_ = Eigen::VectorXd::Zero(num_action_joint_control_);
    act_in_action_eigen_ = Eigen::VectorXd::Zero(num_action_joint_control_);
    act_in_gait_x_ = Eigen::VectorXd::Zero(2);
    act_in_gait_y_ = Eigen::VectorXd::Zero(2);

    act_in_len_ = num_actor_obs_ * num_actor_hist_;
    act_in_ = std::vector<double>(act_in_len_, 0.0);

    act_out_tensor_ = torch::zeros({1, num_action_joint_control_});
    eigen_act_out_raw_ = Eigen::VectorXd::Zero(num_action_joint_control_);
    eigen_act_out_ = Eigen::VectorXd::Zero(num_action_joint_control_);
    eigen_act_out_scale_param_ = act_scale_action_;

    act_out_q_pos_max_ = simple_loco_parameter_->actor_output_max;
    act_out_q_pos_min_ = simple_loco_parameter_->actor_output_min;
    act_out_max_ = act_out_q_pos_max_ + simple_loco_parameter_->actor_output_clip_margin_max;
    act_out_min_ = act_out_q_pos_min_ + simple_loco_parameter_->actor_output_clip_margin_min;

    Eigen::VectorXd init_output = Eigen::VectorXd::Zero(num_action_joint_control_);
    initBuffer(init_output);

    Eigen::VectorXd zeroXd = Eigen::VectorXd::Zero(num_actor_obs_);
    term_hist_deque_ = std::make_unique<SuperDeque>(num_actor_hist_,zeroXd);
}

void SimpleLocoPolicy::prepareActorInput(){
    act_in_cmds_ = robot_motion_command_;
    act_in_base_ang_vel_ = base_rpy_vel_to_self_;
    act_in_base_proj_grav_ = base_project_gravity_;
    act_in_meas_q_pos_def_diff_ = joint_pos_meas_def_diff_;
    act_in_meas_q_vel_ = joint_vel_meas_;
    act_in_action_eigen_ = eigen_act_out_;
    act_in_gait_x_ = gait_generator_->getXNorm();
    // std::cout << "act_in_gait_x_: " << act_in_gait_x_.transpose() << std::endl;
    act_in_gait_y_ = gait_generator_->getYNorm();
}   

void SimpleLocoPolicy::constructActorObs(){
    Eigen::Vector3d input_commands_scaled = act_in_cmds_.cwiseProduct(obs_scales_command_);
    Eigen::Vector3d input_base_angular_velocity_scaled = act_in_base_ang_vel_ * obs_scales_ang_vel_;
    Eigen::Vector3d input_base_projected_gravity_scaled = act_in_base_proj_grav_ * obs_scale_gravity_;
    Eigen::VectorXd input_measured_joint_pos_def_diff_scaled = act_in_meas_q_pos_def_diff_ * obs_scale_joint_pos_;
    Eigen::VectorXd input_measured_joint_velocity_scaled = act_in_meas_q_vel_ * obs_scale_joint_vel_;
    Eigen::VectorXd input_action_scaled = act_in_action_eigen_ * obs_scale_action_;
    // std::cout << "act_in_gait_x_: " << act_in_gait_x_.transpose() << std::endl;
    Eigen::Vector2d input_gait_x = act_in_gait_x_;
    Eigen::Vector2d input_gait_y = act_in_gait_y_;
    Eigen::VectorXd current_input = Eigen::VectorXd::Zero(num_actor_obs_);
    current_input << input_commands_scaled, 
                    input_base_angular_velocity_scaled, 
                    input_base_projected_gravity_scaled, 
                    input_measured_joint_pos_def_diff_scaled, 
                    input_measured_joint_velocity_scaled, 
                    input_action_scaled, 
                    input_gait_x, 
                    input_gait_y;

    term_hist_deque_->push(current_input);
    act_in_ = term_hist_deque_->toDoubleVector();
    act_in_tensor_ = torch::tensor(act_in_).to(*device_);
}

void SimpleLocoPolicy::executeActorNN(){
    act_out_tensor_ = policy_.forward({act_in_tensor_}).toTensor().to(*device_);
    auto act_out_raw_tensor = act_out_tensor_.detach();
    std::vector<float> result(act_out_raw_tensor.data_ptr<float>(), act_out_raw_tensor.data_ptr<float>() + act_out_raw_tensor.numel());
    for(int i = 0; i < result.size(); i++) {
        eigen_act_out_raw_[i] = double(result[i]);
    }

    eigen_act_out_ = eigen_act_out_raw_.cwiseMin(act_out_max_).cwiseMax(act_out_min_);
    eigen_act_out_scaled_ = eigen_act_out_.cwiseProduct(eigen_act_out_scale_param_);
}

void SimpleLocoPolicy::processActorOutput(){
    target_joint_position_offset_ = eigen_act_out_scaled_;
    action_target_joint_position_ = control_joint_default_pos_ + target_joint_position_offset_;
}

void SimpleLocoPolicy::runActor(){
    prepareActorInput();
    constructActorObs();
    executeActorNN();
    processActorOutput();
}


void SimpleLocoPolicy::initBuffer(const Eigen::VectorXd& init_output){
    double epsilon = 1e-8;
    auto diff = init_output - control_joint_default_pos_;
    Eigen::VectorXd scale_with_eps = eigen_act_out_scale_param_.array() + epsilon;
    eigen_act_out_ = diff.array() / scale_with_eps.array();
}

void SimpleLocoPolicy::updateBuffer(RobotData& data){
    grx_sot::robot::RobotState robot_state_fixed = data.hardwareData.getRobotStateFixed();
    CommandFilter(robot_motion_command_);

    robot_motion_command_[0] = std::clamp(robot_motion_command_[0], command_vel_x_range_[0], command_vel_x_range_[1]);
    robot_motion_command_[1] = std::clamp(robot_motion_command_[1], command_vel_y_range_[0], command_vel_y_range_[1]);
    robot_motion_command_[2] = std::clamp(robot_motion_command_[2], command_vel_yaw_range_[0], command_vel_yaw_range_[1]);

    Eigen::VectorXd quat_wxyz = data.baseData.quat_wxyz;
    base_quat_to_world_ = Eigen::Quaterniond(quat_wxyz[0], quat_wxyz[1], quat_wxyz[2], quat_wxyz[3]);
    base_project_gravity_ = data.baseData.grav_proj;
    base_rpy_vel_to_self_ = data.baseData.omega_B;
    joint_pos_meas_ = robot_state_fixed.q.head(num_action_joint_control_);
    joint_vel_meas_ = robot_state_fixed.qd.head(num_action_joint_control_);
    joint_pos_meas_def_diff_ = joint_pos_meas_ - control_joint_default_pos_;

}


void SimpleLocoPolicy::updateSimpleLocoState(){
    // 更新行走/站立状态
    bool is_stand_vel_cmd_condition = (robot_motion_command_.head(2).norm() <= 0.1) && 
                                      (std::abs(robot_motion_command_[2]) <= 0.1);

    if (is_stand_vel_cmd_condition == true) {
        stand_hold_time_ += dt_;
        if (stand_hold_time_ >= 1.5 * gait_generator_->gait_period_) {
            simple_loco_state_ = SimpleLocoState::STAND;
        }
    } else {
        stand_hold_time_ = 0;
    }

    auto gait_pattern = gait_generator_->getCurrentGaitPattern();
    if (simple_loco_state_ == SimpleLocoState::STAND && gait_pattern != GaitPatterns::GAIT_PATTERN_STAND) {
        gait_generator_->setGaitPattern(GaitPatterns::GAIT_PATTERN_STAND);
    }


    bool is_walk_vel_cmd_condition = (robot_motion_command_.head(2).norm() > 0.1) || 
                                     (std::abs(robot_motion_command_[2]) > 0.1);

    if (is_walk_vel_cmd_condition == true) {
        simple_loco_state_ = SimpleLocoState::WALK;
    }

    if (simple_loco_state_ == SimpleLocoState::WALK && gait_pattern != GaitPatterns::GAIT_PATTERN_WALK) {
        gait_generator_->setGaitPattern(GaitPatterns::GAIT_PATTERN_WALK);
    }
}


void SimpleLocoPolicy::CommandFilter(Eigen::VectorXd& command){
    command = command_filter_alpha_ * command + (1 - command_filter_alpha_) * command;
    command = (command.array().abs() < 0.0001).select(Eigen::VectorXd::Zero(command.size()), command);
}


