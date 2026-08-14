#include <algorithm>
#include <chrono>
#include <exception>
#include <iostream>
#include <string_view>
#include <vector>

#include "fourier_aurora_sdk/aurora_client.hpp"

namespace sdk = fourier::aurora_sdk;

namespace {

bool validSourceName(std::string_view name) {
    return !name.empty() && name.size() <= 64 &&
           std::all_of(name.begin(), name.end(), [](unsigned char c) {
               return (c >= 'A' && c <= 'Z') || (c >= 'a' && c <= 'z') ||
                      (c >= '0' && c <= '9') || c == '.' || c == '_' || c == '-';
           });
}

void printResult(
    const char* operation,
    const sdk::OperationResult& result) {
    std::cerr << operation << " failed: code=" << result.code
              << " message=\"" << result.message << "\"\n";
}

template <typename Range>
void printRange(const Range& values) {
    std::cout << "[";
    bool first = true;
    for (const auto& value : values) {
        if (!first) {
            std::cout << ", ";
        }
        first = false;
        std::cout << value;
    }
    std::cout << "]";
}

}  // namespace

int main(int argc, char** argv) {
    if (argc != 2 || !validSourceName(argv[1])) {
        std::cerr << "Usage: " << argv[0] << " <source_name>\n";
        return 2;
    }
    auto& client = sdk::AuroraClient::getInstance();
    try {
        auto options = client.config();
        options.source_name = argv[1];
        options.enabled_endpoints = {
            sdk::Endpoint::AuroraStateSubscriber,
            sdk::Endpoint::EstimatorDataSubscriber,
            sdk::Endpoint::ControlGroupStateSubscriber,
        };
        client.configure(options);
        client.start();

        if (!client.waitForEndpoints(std::chrono::seconds(5))) {
            std::cerr << "GR3 information endpoints were not ready within 5 s.\n";
            client.stop();
            return 1;
        }

        sdk::AuroraState aurora_state;
        auto result = client.getAuroraState(aurora_state);
        if (!result.success()) {
            printResult("getAuroraState", result);
            client.stop();
            return 1;
        }

        sdk::EstimatorData estimator;
        result = client.getEstimatorData(estimator);
        if (!result.success()) {
            printResult("getEstimatorData", result);
            client.stop();
            return 1;
        }

        sdk::ControlGroupState control_groups;
        result = client.getControlGroupState(control_groups);
        if (!result.success()) {
            printResult("getControlGroupState", result);
            client.stop();
            return 1;
        }

        // Aurora SDK numeric state uses SI units unless a field says otherwise.
        std::cout << "robot_type: " << aurora_state.robot_info.robot_type << "\n"
                  << "hardware_type: "
                  << aurora_state.robot_info.hardware_type << "\n"
                  << "end_effector_type: "
                  << aurora_state.robot_info.end_effector_type << "\n"
                  << "fsm_state: " << aurora_state.current_state.name
                  << " (" << aurora_state.current_state.id << ")\n"
                  << "base_rpy_rad: ";
        printRange(estimator.rpy);
        std::cout << "\nbase_velocity_world_mps: ";
        printRange(estimator.vel_W);
        std::cout << "\nstable_level: "
                  << static_cast<int>(estimator.stable_level)
                  << "\ncontrol_groups:";
        for (const auto& group : control_groups.joint_state) {
            std::cout << " " << group.group_name;
        }
        std::cout << "\n";

        client.stop();
        return 0;
    } catch (const std::exception& error) {
        std::cerr << "GR3 get_info failed: " << error.what() << "\n";
        try {
            client.stop();
        } catch (const std::exception& stop_error) {
            std::cerr << "stop failed: " << stop_error.what() << "\n";
        }
        return 1;
    }
}
