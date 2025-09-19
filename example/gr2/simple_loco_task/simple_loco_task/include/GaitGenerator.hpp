/**
 * @file GaitGenerator.hpp
 * @brief Header file for GaitGenerator in robot controllers.
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

#include <vector>
#include <deque>
#include <cmath>
#include <random>
#include <Eigen/Dense>  
#include "ParameterManager.hpp"
#include "SimpleLocoParameter.hpp"
#include <robot_data/RobotData.hpp> 

enum GaitPatterns {
    GAIT_PATTERN_OFF = 0,
    GAIT_PATTERN_STAND = 1,
    GAIT_PATTERN_WALK = 2,
    GAIT_PATTERN_RUN = 3,
    GAIT_PATTERN_JUMP = 4,
    GAIT_PATTERN_HOP = 5
};
class GaitGenerator {

public:
    GaitGenerator(SimpleLocoParameter* simple_loco_parameter);

    ~GaitGenerator(){

    }
    
    void init(RobotData& data);
    void reset(bool randomize = false);
    void step();


    void setX(const Eigen::VectorXd& x){
        x_ = x;
    }
    void setY(const Eigen::VectorXd& y){
        y_ = y;
    }
    void setPhaseOffsetTarget(const Eigen::VectorXd& phase_offset_target){
        phase_offset_target_ = phase_offset_target;
    }
    void setFrequency(const double& frequency){
        gait_freq_ = frequency;
    }
    void setCycleR(const Eigen::VectorXd& cycle_r){
        cycle_r_ = cycle_r;
    }
    void setCouplingChangeRate(const double& coupling_change_rate){
        coupling_change_rate_ = coupling_change_rate;
    }
    void setAmplitudeChangeRate(const double& amplitude_change_rate){
        amplitude_change_rate_ = amplitude_change_rate;
    }
    void setContactRatio(const Eigen::VectorXd& contact_ratio){
        contact_ratio_ = contact_ratio;
    }
    void setGaitPattern(GaitPatterns pattern_type);

    void fromStandToMove(const Eigen::VectorXd& phase_offset);

    GaitPatterns getCurrentGaitPattern() { return gait_patterns_; }
    const Eigen::VectorXd& getX();
    const Eigen::VectorXd& getY();
    const Eigen::VectorXd& getXNorm();
    const Eigen::VectorXd& getYNorm();

public:

    double gait_ctrl_dt_; 
    double gait_period_; 
    double gait_freq_;

    int num_feet_;
    double reference_speed_;

    GaitPatterns gait_patterns_;

    Eigen::VectorXd x_;
    Eigen::VectorXd y_;
    Eigen::VectorXd x_norm_;
    Eigen::VectorXd y_norm_;
    double rate_xy_;

    Eigen::VectorXd cycle_r_;
    double cycle_r_max_;
    double cycle_r_min_;

    double coupling_change_rate_;
    double amplitude_change_rate_;
    double coff_b_;

    Eigen::VectorXd phase_offset_;
    Eigen::VectorXd phase_offset_target_;
    double phase_offset_change_rate_;

    Eigen::VectorXd contact_ratio_;
    double contact_threshold_;
    Eigen::VectorXd foot_contact_;

    SimpleLocoParameter* simple_loco_parameter_;
};
