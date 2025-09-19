/**
 * @file PolicyParent.hpp
 * @brief Header file for PolicyParent in robot controllers.
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

#include "ParameterManager.hpp"
#include "robot_data/RobotData.hpp"

class PolicyParent {
public:
    virtual ~PolicyParent() = default;
    virtual void init(RobotData& data) = 0;
    virtual void enter(RobotData& data) = 0;
    virtual void exit(RobotData& data) = 0;
    virtual void runPolicy(RobotData& data) = 0;
    ParameterManager& config = ParameterManager::get_instance();
};

