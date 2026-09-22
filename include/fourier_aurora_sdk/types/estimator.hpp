#pragma once

#include <array>
#include <cstdint>
#include <string>
#include <vector>

#include "fourier_aurora_sdk/types/common.hpp"

namespace fourier::aurora_sdk {

struct BaseData {
    MessageHeader header;
    std::array<double, 4> quat_xyzw{0.0, 0.0, 0.0, 0.0};
    std::array<double, 4> quat_wxyz{0.0, 0.0, 0.0, 0.0};
    std::array<double, 3> rpy{0.0, 0.0, 0.0};
    std::array<double, 3> omega_W{0.0, 0.0, 0.0};
    std::array<double, 3> acc_W{0.0, 0.0, 0.0};
    std::array<double, 3> omega_B{0.0, 0.0, 0.0};
    std::array<double, 3> acc_B{0.0, 0.0, 0.0};
    std::array<double, 3> vel_W{0.0, 0.0, 0.0};
    std::array<double, 3> pos_W{0.0, 0.0, 0.0};
    std::array<double, 3> vel_B{0.0, 0.0, 0.0};
    uint8_t stable_level = 0;
    // Appended to preserve existing aggregate initializers. Empty for legacy messages.
    // Robot link whose pose is estimated; distinct from header.frame_id.
    std::string root_link;
};

struct ContactData {
    MessageHeader header;
    std::vector<double> contact_force;
    std::vector<double> contact_prob;
};

struct EstimatorData {
    MessageHeader header;
    std::array<double, 4> quat_xyzw{0.0, 0.0, 0.0, 0.0};
    std::array<double, 4> quat_wxyz{0.0, 0.0, 0.0, 0.0};
    std::array<double, 3> rpy{0.0, 0.0, 0.0};
    std::array<double, 3> omega_W{0.0, 0.0, 0.0};
    std::array<double, 3> acc_W{0.0, 0.0, 0.0};
    std::array<double, 3> omega_B{0.0, 0.0, 0.0};
    std::array<double, 3> acc_B{0.0, 0.0, 0.0};
    std::array<double, 3> vel_W{0.0, 0.0, 0.0};
    std::array<double, 3> pos_W{0.0, 0.0, 0.0};
    std::array<double, 3> vel_B{0.0, 0.0, 0.0};
    std::vector<double> contact_force;
    std::vector<double> contact_prob;
    uint8_t stable_level = 0;
    // Appended to preserve existing aggregate initializers. Empty for legacy messages.
    // Robot link whose pose is estimated; distinct from header.frame_id.
    std::string root_link;
};

}  // namespace fourier::aurora_sdk
