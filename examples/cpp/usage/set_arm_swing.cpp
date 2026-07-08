#include <chrono>
#include <cmath>
#include <cstdlib>
#include <exception>
#include <iostream>
#include <string>
#include <thread>
#include <vector>

#include "example_common.hpp"
#include "fourier_aurora_sdk/aurora_client.hpp"

namespace {

constexpr uint32_t kPdStandHanging = 12;
constexpr uint32_t kPdStandStanding = 13;
constexpr double kControlPeriodSec = 0.01;
constexpr int kInterpolationSteps = 300;
constexpr int kHoldSteps = 100;
constexpr int kSwingPeriodSteps = 500;
constexpr double kPi = 3.14159265358979323846;
constexpr size_t kShoulderRollIndex = 1;
constexpr size_t kElbowIndex = 3;

std::vector<double> getArmPosition(
    fourier::aurora_sdk::AuroraClient& client,
    const std::string& group_name) {
    try {
        return client.getJointCommand(group_name, fourier::aurora_sdk::JointField::Position);
    } catch (const std::exception&) {
        return client.getJointState(group_name, fourier::aurora_sdk::JointField::Position);
    }
}

fourier::aurora_sdk::JointCommand makeJointCommand(
    const std::string& group_name,
    const std::vector<double>& position) {
    fourier::aurora_sdk::JointCommand command;
    command.group_name = group_name;
    command.control_type = fourier::aurora_sdk::JointControlType::Joint;
    command.motor_mode = fourier::aurora_sdk::MotorMode::Pd;
    command.position = position;
    command.velocity.assign(position.size(), 0.0);
    command.effort.assign(position.size(), 0.0);
    return command;
}

void publishArms(
    fourier::aurora_sdk::AuroraClient& client,
    const std::vector<double>& left_position,
    const std::vector<double>& right_position) {
    fourier::aurora_sdk::ControlGroupCommand command;
    command.joint_cmd.push_back(makeJointCommand("left_manipulator", left_position));
    command.joint_cmd.push_back(makeJointCommand("right_manipulator", right_position));
    client.publishControlGroupCommand(command);
}

bool hasArmJoints(const std::vector<double>& position) {
    return position.size() > kElbowIndex;
}

}  // namespace

int main(int argc, char** argv) {
    example::printUsage(argv[0]);
    const auto config = example::parseConfig(argc, argv, "cpp_set_arm_swing");
    example::installStopSignalHandlers();

    fourier::aurora_sdk::AuroraClient* client = nullptr;
    try {
        client = &fourier::aurora_sdk::AuroraClient::getInstance(
            config.user_id,
            config.domain_id);
        std::this_thread::sleep_for(std::chrono::seconds(1));

        const auto switch_result = client->requestFsmState(kPdStandHanging);
        std::cout << "requestFsmState(" << kPdStandHanging << ") ";
        example::printServiceResult(switch_result);
        if (!switch_result.success()) {
            std::cout << "Continue anyway; the robot may already be in PdStandStanding.\n";
        }

        if (!example::waitForEnter(
                "Place the robot onto the ground. Press Enter after it switches to PdStandStanding (13)...")) {
            client->close();
            return 1;
        }

        const auto aurora_state = client->getAuroraState();
        if (aurora_state.current_state.id != kPdStandStanding) {
            std::cerr << "Robot is not in PdStandStanding (13). Current state is "
                      << aurora_state.current_state.id << " (" << aurora_state.current_state.name << ").\n"
                      << "Place the robot onto the ground again and rerun this example.\n";
            std::cerr.flush();
            std::_Exit(1);
        }

        const auto left_start = getArmPosition(*client, "left_manipulator");
        const auto right_start = getArmPosition(*client, "right_manipulator");
        if (!hasArmJoints(left_start) || !hasArmJoints(right_start)) {
            std::cerr << "Arm control groups must contain at least shoulder roll and elbow joints.\n";
            std::cerr.flush();
            std::_Exit(1);
        }

        std::vector<double> left_zero(left_start.size(), 0.0);
        std::vector<double> right_zero(right_start.size(), 0.0);

        auto left_target = left_zero;
        auto right_target = right_zero;
        left_target[kShoulderRollIndex] = 0.2;
        right_target[kShoulderRollIndex] = -0.2;
        left_target[kElbowIndex] = -0.7;
        right_target[kElbowIndex] = -0.7;

        const auto dt = std::chrono::duration<double>(kControlPeriodSec);
        const auto sleep_dt = std::chrono::duration_cast<std::chrono::steady_clock::duration>(dt);

        for (int i = 1; i <= kInterpolationSteps && !example::stopRequested(); ++i) {
            const double alpha = static_cast<double>(i) / static_cast<double>(kInterpolationSteps);
            auto left_position = left_start;
            auto right_position = right_start;
            for (size_t j = 0; j < left_position.size(); ++j) {
                left_position[j] = left_start[j] + alpha * (left_target[j] - left_start[j]);
            }
            for (size_t j = 0; j < right_position.size(); ++j) {
                right_position[j] = right_start[j] + alpha * (right_target[j] - right_start[j]);
            }
            publishArms(*client, left_position, right_position);
            std::this_thread::sleep_for(sleep_dt);
        }

        for (int i = 0; i < kHoldSteps && !example::stopRequested(); ++i) {
            publishArms(*client, left_target, right_target);
            std::this_thread::sleep_for(sleep_dt);
        }

        int swing_step = 0;
        while (!example::stopRequested()) {
            const double theta = 2.0 * kPi *
                static_cast<double>(swing_step % kSwingPeriodSteps) /
                static_cast<double>(kSwingPeriodSteps);

            auto left_position = left_target;
            auto right_position = right_target;
            left_position[kElbowIndex] = -0.7 + 0.7 * std::sin(theta);
            right_position[kElbowIndex] = -0.7 + 0.7 * std::sin(theta + kPi);
            publishArms(*client, left_position, right_position);
            ++swing_step;
            std::this_thread::sleep_for(sleep_dt);
        }

        client->close();
    } catch (const std::exception& error) {
        if (client != nullptr) {
            client->close();
        }
        std::cerr << "set_arm_swing failed: " << error.what() << "\n";
        return 1;
    }
}
