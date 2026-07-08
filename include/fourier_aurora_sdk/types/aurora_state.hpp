#pragma once

#include <cstdint>
#include <string>
#include <vector>

#include "fourier_aurora_sdk/types/common.hpp"

namespace fourier::aurora_sdk {

struct RobotInfo {
    std::string robot_type;
    std::string hardware_type;
    std::string end_effector_type;
};

struct StateInfo {
    uint32_t id = 0;
    uint32_t version = 0;
    std::string name;
    std::string description;
    bool active = false;
};

struct AuroraState {
    MessageHeader header;
    RobotInfo robot_info;
    uint8_t switch_status = 0;
    StateInfo current_state;
    std::vector<StateInfo> available_states;
};

}  // namespace fourier::aurora_sdk
