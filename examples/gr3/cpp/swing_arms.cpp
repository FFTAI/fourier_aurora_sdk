#include <algorithm>
#include <chrono>
#include <cmath>
#include <csignal>
#include <exception>
#include <iostream>
#include <string>
#include <string_view>
#include <thread>
#include <vector>

#include "fourier_aurora_sdk/aurora_client.hpp"

namespace sdk = fourier::aurora_sdk;

namespace {

inline constexpr uint32_t kPdStandStandingState = 13;
inline constexpr std::size_t kShoulderRollIndex = 1;
inline constexpr std::size_t kElbowIndex = 3;
inline constexpr double kInterpolationSeconds = 3.0;
inline constexpr double kSwingPeriodSeconds = 5.0;
inline constexpr double kSwingSeconds = 10.0;
inline constexpr double kControlPeriodSeconds = 0.01;
inline constexpr double kPi = 3.14159265358979323846;

volatile std::sig_atomic_t interrupted = 0;

void handleSignal(int) {
    interrupted = 1;
}

void printResult(
    const char* operation,
    const sdk::OperationResult& result) {
    std::cerr << operation << " failed: code=" << result.code
              << " message=\"" << result.message << "\"\n";
}

bool validSourceName(std::string_view name) {
    return !name.empty() && name.size() <= 64 &&
           std::all_of(name.begin(), name.end(), [](unsigned char c) {
               return (c >= 'A' && c <= 'Z') || (c >= 'a' && c <= 'z') ||
                      (c >= '0' && c <= '9') || c == '.' || c == '_' || c == '-';
           });
}

const sdk::ControlGroupStateEntry* findGroup(
    const sdk::ControlGroupState& state,
    const std::string& name) {
    const auto found = std::find_if(
        state.joint_state.begin(),
        state.joint_state.end(),
        [&](const sdk::ControlGroupStateEntry& group) {
            return group.group_name == name;
        });
    return found == state.joint_state.end() ? nullptr : &*found;
}

sdk::JointCommand makeJointCommand(
    const std::string& group_name,
    const std::vector<double>& position) {
    sdk::JointCommand command;
    command.group_name = group_name;
    command.control_type = sdk::JointControlType::Joint;
    command.motor_mode = sdk::MotorMode::Pd;
    command.position = position;
    command.velocity.assign(position.size(), 0.0);
    command.effort.assign(position.size(), 0.0);
    return command;
}

bool publishArms(
    sdk::AuroraClient& client,
    const std::vector<double>& left,
    const std::vector<double>& right) {
    sdk::ControlGroupCommand command;
    command.joint_cmd = {
        makeJointCommand("left_manipulator", left),
        makeJointCommand("right_manipulator", right),
    };
    const auto result = client.publishControlGroupCommand(command);
    if (!result.success()) {
        printResult("publishControlGroupCommand", result);
        return false;
    }
    return true;
}

bool interpolateToCenter(
    sdk::AuroraClient& client,
    const std::vector<double>& left_start,
    const std::vector<double>& right_start,
    const std::vector<double>& left_center,
    const std::vector<double>& right_center) {
    const auto begin = std::chrono::steady_clock::now();
    std::size_t step = 0;
    for (;;) {
        if (interrupted) {
            return false;
        }
        const auto elapsed = std::chrono::steady_clock::now() - begin;
        const double alpha = std::min(
            1.0,
            std::chrono::duration<double>(elapsed).count() /
                kInterpolationSeconds);
        auto left = left_start;
        auto right = right_start;
        for (std::size_t joint = 0; joint < left.size(); ++joint) {
            left[joint] =
                left_start[joint] +
                alpha * (left_center[joint] - left_start[joint]);
        }
        for (std::size_t joint = 0; joint < right.size(); ++joint) {
            right[joint] =
                right_start[joint] +
                alpha * (right_center[joint] - right_start[joint]);
        }
        if (!publishArms(client, left, right)) {
            return false;
        }
        if (alpha >= 1.0) {
            return true;
        }
        ++step;
        std::this_thread::sleep_until(
            begin +
            std::chrono::duration_cast<
                std::chrono::steady_clock::duration>(
                std::chrono::duration<double>(
                    kControlPeriodSeconds * step)));
    }
}

bool swingArms(
    sdk::AuroraClient& client,
    const std::vector<double>& left_center,
    const std::vector<double>& right_center) {
    const auto begin = std::chrono::steady_clock::now();
    std::size_t step = 0;
    while (!interrupted) {
        const double elapsed = std::chrono::duration<double>(
                                   std::chrono::steady_clock::now() - begin)
                                   .count();
        if (elapsed >= kSwingSeconds) {
            break;
        }
        const double offset =
            0.7 * std::sin(2.0 * kPi * elapsed / kSwingPeriodSeconds);
        auto left = left_center;
        auto right = right_center;
        left[kElbowIndex] += offset;
        right[kElbowIndex] -= offset;
        if (!publishArms(client, left, right)) {
            return false;
        }
        ++step;
        std::this_thread::sleep_until(
            begin +
            std::chrono::duration_cast<
                std::chrono::steady_clock::duration>(
                std::chrono::duration<double>(
                    kControlPeriodSeconds * step)));
    }
    return !interrupted &&
           publishArms(client, left_center, right_center);
}

void stopAfterFailure(sdk::AuroraClient& client) {
    try {
        client.stop();
    } catch (const std::exception& error) {
        std::cerr << "stop failed: " << error.what() << "\n";
    }
}

}  // namespace

int main(int argc, char** argv) {
    if (argc != 2 || !validSourceName(argv[1])) {
        std::cerr << "Usage: " << argv[0] << " <source_name>\n";
        return 2;
    }
    const std::string source_name = argv[1];

    std::cout
        << "GR3 swing_arms will request PdStandStanding (FSM 13), "
        << "interpolate both arms for " << kInterpolationSeconds
        << " s, then run two " << kSwingPeriodSeconds
        << " s swing periods. The motion lasts about "
        << (kInterpolationSeconds + kSwingSeconds) << " s.\n"
        << "Confirm the robot is supported and the area is clear. "
        << "Type YES to continue: ";
    std::string confirmation;
    if (!std::getline(std::cin, confirmation) || confirmation != "YES") {
        std::cout << "Cancelled.\n";
        return 0;
    }

    std::signal(SIGINT, handleSignal);
    std::signal(SIGTERM, handleSignal);

    auto& client = sdk::AuroraClient::getInstance();
    try {
        auto options = client.config();
        options.source_name = source_name;
        options.enabled_endpoints = {
            sdk::Endpoint::FsmService,
            sdk::Endpoint::ManageLeaseService,
            sdk::Endpoint::AuroraStateSubscriber,
            sdk::Endpoint::ControlGroupStateSubscriber,
            sdk::Endpoint::ControlGroupCommandPublisher,
        };
        client.configure(options);
        client.start();

        if (!client.waitForEndpoints(std::chrono::seconds(5))) {
            std::cerr << "GR3 arm endpoints were not ready within 5 s.\n";
            client.stop();
            return interrupted ? 130 : 1;
        }
        if (interrupted) {
            client.stop();
            return 130;
        }

        const auto lease = client.registerLease({
            sdk::AuroraResource::LeftManipulator,
            sdk::AuroraResource::RightManipulator,
        });
        if (!lease.usable()) {
            printResult("registerLease(manipulators)", lease.operation);
            client.stop();
            return 1;
        }

        auto result = client.requestFsmState(kPdStandStandingState);
        if (!result.success()) {
            printResult("requestFsmState(PdStandStanding)", result);
            client.stop();
            return 1;
        }
        if (!client.waitForFsmState(
                kPdStandStandingState, std::chrono::seconds(10))) {
            std::cerr
                << "GR3 did not enter PdStandStanding (FSM 13) within 10 s.\n";
            client.stop();
            return interrupted ? 130 : 1;
        }

        sdk::ControlGroupState state;
        result = client.getControlGroupState(state);
        if (!result.success()) {
            printResult("getControlGroupState", result);
            client.stop();
            return 1;
        }

        // Both starts come from this one state snapshot.
        const auto* left = findGroup(state, "left_manipulator");
        const auto* right = findGroup(state, "right_manipulator");
        if (!left || !right ||
            left->joint_position.size() <= kElbowIndex ||
            right->joint_position.size() <= kElbowIndex) {
            std::cerr
                << "Both GR3 manipulator groups must contain shoulder-roll "
                << "index 1 and elbow index 3.\n";
            client.stop();
            return 1;
        }

        std::vector<double> left_center(
            left->joint_position.size(), 0.0);
        std::vector<double> right_center(
            right->joint_position.size(), 0.0);
        left_center[kShoulderRollIndex] = 0.2;
        right_center[kShoulderRollIndex] = -0.2;
        left_center[kElbowIndex] = -0.7;
        right_center[kElbowIndex] = -0.7;

        if (!interpolateToCenter(
                client,
                left->joint_position,
                right->joint_position,
                left_center,
                right_center) ||
            !swingArms(client, left_center, right_center)) {
            client.stop();
            return interrupted ? 130 : 1;
        }

        const auto release = client.releaseLease();
        if (!release.success()) {
            printResult("releaseLease", release.operation);
        }
        client.stop();
        return 0;
    } catch (const std::exception& error) {
        std::cerr << "GR3 swing_arms failed: " << error.what() << "\n";
        stopAfterFailure(client);
        return interrupted ? 130 : 1;
    }
}
