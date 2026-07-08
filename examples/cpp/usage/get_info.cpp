#include <chrono>
#include <exception>
#include <iostream>
#include <thread>

#include "example_common.hpp"
#include "fourier_aurora_sdk/aurora_client.hpp"

int main(int argc, char** argv) {
    example::printUsage(argv[0]);
    const auto config = example::parseConfig(argc, argv, "cpp_get_info");

    try {
        auto& client = fourier::aurora_sdk::AuroraClient::getInstance(
            config.user_id,
            config.domain_id);
        std::this_thread::sleep_for(std::chrono::seconds(1));

        while (true) {
            try {
                const auto aurora_state = client.getAuroraState();
                const auto base_data = client.getBaseData();
                const auto stable_level = client.getStableLevel();
                const auto left_leg_position = client.getJointState(
                    "left_leg",
                    fourier::aurora_sdk::JointField::Position);

                std::cout << "robot_type=" << aurora_state.robot_info.robot_type
                          << " state_id=" << aurora_state.current_state.id
                          << " state_name=" << aurora_state.current_state.name
                          << " base_rpy=";
                example::printArray3(base_data.rpy);
                std::cout << " stable_level=" << static_cast<int>(stable_level)
                          << " left_leg_position=";
                example::printVector(left_leg_position);
                std::cout << "\n";
            } catch (const std::exception& error) {
                std::cerr << "waiting for state data: " << error.what() << "\n";
            }

            std::this_thread::sleep_for(std::chrono::seconds(1));
        }
    } catch (const std::exception& error) {
        std::cerr << "get_info failed: " << error.what() << "\n";
        return 1;
    }
}
