#pragma once

#include <cstdint>
#include <string>
#include <vector>

#include "fourier_aurora_sdk/types/common.hpp"

namespace fourier::aurora_sdk {

enum class MotorConfigurationMode : uint8_t {
    Disable = 0,
    Position = 1,
    Velocity = 2,
    Torque = 3,
    Pd = 4,
};

struct MotorConfigurationGroupCommand {
    std::string group_name;
    MotorConfigurationMode mode{MotorConfigurationMode::Disable};
    std::vector<double> position_kp;
    std::vector<double> position_ki;
    std::vector<double> position_kd;
    std::vector<double> pd_kp;
    std::vector<double> pd_kd;
};

// Temporary Aurora SDK surface while the hardware SDK is unavailable. This
// configuration API will move to the hardware-level SDK later.
struct MotorConfigurationCommand {
    MessageHeader header;
    std::vector<MotorConfigurationGroupCommand> groups;
};

struct MotorConfigurationGroupState {
    std::string group_name;
    MotorConfigurationMode mode{MotorConfigurationMode::Disable};
    std::vector<uint32_t> motor_error;
    std::vector<double> position_kp;
    std::vector<double> position_ki;
    std::vector<double> position_kd;
    std::vector<double> pd_kp;
    std::vector<double> pd_kd;
};

// Temporary Aurora SDK surface while the hardware SDK is unavailable.
struct MotorConfigurationState {
    MessageHeader header;
    std::vector<MotorConfigurationGroupState> groups;
};

}  // namespace fourier::aurora_sdk
