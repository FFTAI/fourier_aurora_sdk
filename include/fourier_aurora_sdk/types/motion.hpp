#pragma once

#include <array>
#include <cstdint>

#include "fourier_aurora_sdk/types/common.hpp"

namespace fourier::aurora_sdk {

struct MotionState {
    MessageHeader header;
    std::array<double, 3> position{0.0, 0.0, 0.0};
    std::array<double, 3> position_cmd{0.0, 0.0, 0.0};
    std::array<double, 3> lin_velocity{0.0, 0.0, 0.0};
    std::array<double, 3> lin_velocity_cmd{0.0, 0.0, 0.0};
    std::array<double, 3> attitude{0.0, 0.0, 0.0};
    std::array<double, 3> attitude_cmd{0.0, 0.0, 0.0};
    std::array<double, 3> ang_velocity{0.0, 0.0, 0.0};
    std::array<double, 3> ang_velocity_cmd{0.0, 0.0, 0.0};
    double delta_height = 0.0;
    double delta_height_cmd = 0.0;
    uint8_t progress = 0;
    uint8_t stable_level = 0;
};

struct MotionCommand {
    MessageHeader header;
    std::array<double, 3> position{0.0, 0.0, 0.0};
    std::array<double, 3> lin_velocity{0.0, 0.0, 0.0};
    std::array<double, 3> attitude{0.0, 0.0, 0.0};
    std::array<double, 3> ang_velocity{0.0, 0.0, 0.0};
    double delta_height = 0.0;
    uint8_t gait_type = 0;
    std::array<uint8_t, 8> skills{0, 0, 0, 0, 0, 0, 0, 0};
    double duration = 1.0;
};

}  // namespace fourier::aurora_sdk
