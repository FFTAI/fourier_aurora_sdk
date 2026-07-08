#include <chrono>
#include <exception>
#include <iostream>
#include <thread>

#include "example_common.hpp"
#include "fourier_aurora_sdk/aurora_client.hpp"

namespace {

void requestStateWithConfirmation(
    fourier::aurora_sdk::AuroraClient& client,
    uint32_t state_id,
    const std::string& state_name) {
    example::waitForEnter("Press Enter to switch to " + state_name + " (" + std::to_string(state_id) + ")...");
    const auto result = client.requestFsmState(state_id);
    std::cout << "requestFsmState(" << state_id << ") ";
    example::printServiceResult(result);
    example::waitForEnter("Press Enter after confirming " + state_name + " is active...");
}

}  // namespace

int main(int argc, char** argv) {
    example::printUsage(argv[0]);
    const auto config = example::parseConfig(argc, argv, "cpp_set_motion");

    try {
        auto& client = fourier::aurora_sdk::AuroraClient::getInstance(
            config.user_id,
            config.domain_id);
        std::this_thread::sleep_for(std::chrono::seconds(1));

        requestStateWithConfirmation(client, 13, "PdStandStanding");

        std::cout << "setStandPose(delta_height=-0.1, pitch=0.0, yaw=0.0, duration=1.0)\n";
        client.setStandPose(-0.1, 0.0, 0.0, 1.0);
        std::this_thread::sleep_for(std::chrono::seconds(1));

        std::cout << "setStandPose(delta_height=0.0, pitch=0.0, yaw=0.0, duration=1.0)\n";
        client.setStandPose(0.0, 0.0, 0.0, 1.0);

        requestStateWithConfirmation(client, 3, "Walk");

        std::cout << "setVelocity(vx=0.3, vy=0.0, yaw=0.0, duration=5.0)\n";
        client.setVelocity(0.3, 0.0, 0.0, 5.0);
        std::this_thread::sleep_for(std::chrono::seconds(5));

        client.close();
    } catch (const std::exception& error) {
        std::cerr << "set_motion failed: " << error.what() << "\n";
        return 1;
    }
}
