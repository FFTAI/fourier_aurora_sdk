#pragma once

#include <array>
#include <cstdint>
#include <string>
#include <vector>

#include "fourier_aurora_sdk/types/common.hpp"

namespace fourier::aurora_sdk {

enum class JointControlType : uint8_t {
    Joint = 0,
    Motor = 1,
};

enum class MotorMode : uint8_t {
    Pd = 0,
    Position = 1,
    Effort = 2,
};

enum class JointField {
    Position,
    Velocity,
    Effort,
};

struct ControlGroupStateEntry {
    std::string group_name;
    std::vector<double> joint_position;
    std::vector<double> joint_velocity;
    std::vector<double> joint_effort;
};

struct CartesianStateEntry {
    std::string link_name;
    std::array<double, 7> pose{0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0};
    std::array<double, 6> twist{0.0, 0.0, 0.0, 0.0, 0.0, 0.0};
    std::array<double, 6> wrench{0.0, 0.0, 0.0, 0.0, 0.0, 0.0};
};

struct ControlGroupState {
    MessageHeader header;
    std::vector<ControlGroupStateEntry> joint_state;
    std::vector<ControlGroupStateEntry> joint_cmd;
    std::vector<CartesianStateEntry> cartesian_state;
    std::vector<CartesianStateEntry> cartesian_cmd;
};

struct JointCommand {
    std::string group_name;
    JointControlType control_type = JointControlType::Joint;
    MotorMode motor_mode = MotorMode::Pd;
    std::vector<double> position;
    std::vector<double> velocity;
    std::vector<double> effort;
};

struct CartesianCommand {
    std::string link_name;
    std::array<double, 7> pose{0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0};
    std::array<double, 6> twist{0.0, 0.0, 0.0, 0.0, 0.0, 0.0};
    std::array<double, 6> wrench{0.0, 0.0, 0.0, 0.0, 0.0, 0.0};
};

struct ControlGroupCommand {
    MessageHeader header;
    std::vector<JointCommand> joint_cmd;
    std::vector<CartesianCommand> cartesian_cmd;
};

}  // namespace fourier::aurora_sdk
