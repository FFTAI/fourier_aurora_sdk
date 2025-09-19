/**
 * @file SimpleLocoPolicy.hpp
 * @brief Header file for SimpleLocoPolicy in robot controllers.
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

#include <state/Task.hpp>
#include <ParameterManager.hpp>
#include <robot_data/RobotData.hpp>
#include "SuperDeque.hpp"
#include "PolicyParent.hpp"
#include <torch/torch.h>
#include <torch/script.h>
#include "SimpleLocoParameter.hpp"
#include "GaitGenerator.hpp"

// 具体任务实现
class SimpleLocoPolicy : public PolicyParent {
    enum class SimpleLocoState {
        IDLE = 0,
        STAND = 1,
        WALK = 2,
    };
public:
    SimpleLocoPolicy(SimpleLocoParameter* simple_loco_parameter){
        simple_loco_parameter_ = simple_loco_parameter;
    }
    ~SimpleLocoPolicy(){

    }

    void init(RobotData& data) override;
    void enter(RobotData& data) override;
    void exit(RobotData& data) override;
    void runPolicy(RobotData& data) override;

    void initActor();
    void prepareActorInput();
    void constructActorObs();
    void executeActorNN();
    void processActorOutput();
    void runActor();

    void initBuffer(const Eigen::VectorXd& init_output);
    void updateBuffer(RobotData& data);

    void updateSimpleLocoState();


    void CommandFilter(Eigen::VectorXd& command);

    Eigen::VectorXd getActionCommand(){
        return action_target_joint_position_;
    }
    
private:

    double dt_;
    double control_freq_;

    Eigen::VectorXd control_joint_default_pos_;
    Eigen::VectorXd control_joint_kp_;
    Eigen::VectorXd control_joint_kd_;

    double obs_scales_lin_vel_;
    double obs_scales_ang_vel_;
    Eigen::VectorXd obs_scales_command_;
    double obs_scale_gravity_;
    double obs_scale_joint_pos_;
    double obs_scale_joint_vel_;
    double obs_scale_height_measurements_;
    double obs_scale_action_; 
    Eigen::VectorXd act_scale_action_;


    int num_action_joint_control_;
    int num_actor_obs_;
    int num_actor_hist_;
    torch::Device* device_;
    torch::jit::script::Module policy_;

    Eigen::VectorXd act_in_cmds_;
    Eigen::VectorXd act_in_base_ang_vel_;
    Eigen::VectorXd act_in_base_proj_grav_;
    Eigen::VectorXd act_in_meas_q_pos_def_diff_;
    Eigen::VectorXd act_in_meas_q_vel_;
    Eigen::VectorXd act_in_action_eigen_;
    Eigen::VectorXd act_in_gait_x_;
    Eigen::VectorXd act_in_gait_y_;
    int act_in_len_;
    std::vector<double> act_in_;
    torch::Tensor act_in_tensor_;


    torch::Tensor act_out_tensor_;
    Eigen::VectorXd eigen_act_out_raw_;
    Eigen::VectorXd eigen_act_out_;
    Eigen::VectorXd eigen_act_out_scale_param_;
    Eigen::VectorXd eigen_act_out_scaled_;

    Eigen::VectorXd act_out_q_pos_max_;
    Eigen::VectorXd act_out_q_pos_min_;
    
    Eigen::VectorXd act_out_max_;
    Eigen::VectorXd act_out_min_;

    Eigen::VectorXd robot_motion_command_;
    double command_filter_cf_;
    double command_filter_tau_;
    double command_filter_alpha_;

    double command_safety_ratio_;
    Eigen::VectorXd command_vel_x_range_;
    Eigen::VectorXd command_vel_y_range_;
    Eigen::VectorXd command_vel_yaw_range_;

    Eigen::VectorXd joint_pos_meas_;
    Eigen::VectorXd joint_vel_meas_;
    Eigen::VectorXd joint_trq_meas_;
    Eigen::VectorXd joint_pos_meas_def_diff_;

    Eigen::Quaterniond base_ori_quat_to_world_;


    Eigen::VectorXd base_xyz_vel_to_self_;
    Eigen::VectorXd base_rpy_vel_to_self_;
    Eigen::VectorXd base_xyz_vel_to_world_;
    Eigen::Quaterniond base_quat_to_world_;
    Eigen::VectorXd base_project_gravity_;

    Eigen::VectorXd target_joint_position_offset_;

    Eigen::VectorXd action_target_joint_position_;


    std::unique_ptr<SuperDeque> term_hist_deque_;
    
    std::unique_ptr<GaitGenerator> gait_generator_;
    double stand_hold_time_;
    SimpleLocoState simple_loco_state_;

    SimpleLocoParameter* simple_loco_parameter_;
};

