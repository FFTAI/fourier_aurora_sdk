#include <chrono>
#include <iostream>
#include <stdexcept>
#include <thread>
#include "fourier_aurora_sdk/aurora_client.hpp"

namespace sdk = fourier::aurora_sdk;

int main() {
    auto& client = sdk::AuroraClient::getInstance();
    int status = 0;
    try {
        auto options = client.config();
        options.domain_id = 123;  // Match the robot deployment.
        options.source_name = "mini-cpp-state-reader";
        options.enabled_endpoints = {sdk::Endpoint::AuroraStateSubscriber};
        client.configure(options);
        client.start();
        if (!client.waitForEndpoints(std::chrono::seconds(5))) {
            throw std::runtime_error("Aurora endpoint did not match");
        }
        sdk::AuroraState state;
        const auto deadline = std::chrono::steady_clock::now() + std::chrono::seconds(3);
        while (true) {
            const auto result = client.getAuroraState(state);
            if (result.success()) break;
            if (std::chrono::steady_clock::now() >= deadline) {
                throw std::runtime_error(std::to_string(result.code) + ": " + result.message);
            }
            std::this_thread::sleep_for(std::chrono::milliseconds(50));
        }
        std::cout << state.robot_info.robot_type << " " << state.robot_info.hardware_type << "\n";
        std::cout << state.current_state.id << " " << state.current_state.name << "\n";
    } catch (const std::exception& error) {
        std::cerr << error.what() << "\n";
        status = 1;
    }
    try {
        client.stop();
    } catch (const std::exception& error) {
        std::cerr << "Stop failed: " << error.what() << "\n";
        status = 1;
    }
    return status;
}
