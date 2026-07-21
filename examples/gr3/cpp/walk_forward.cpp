#include <algorithm>
#include <chrono>
#include <csignal>
#include <exception>
#include <iostream>
#include <string>
#include <thread>

#include "fourier_aurora_sdk/aurora_client.hpp"

namespace sdk = fourier::aurora_sdk;

namespace {

inline constexpr uint32_t kGr3WalkState = 3;
inline constexpr double kForwardVelocityMps = 0.3;
inline constexpr double kWalkDurationSeconds = 3.0;

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

void stopAfterFailure(sdk::AuroraClient& client) {
    try {
        client.stop();
    } catch (const std::exception& error) {
        std::cerr << "stop failed: " << error.what() << "\n";
    }
}

}  // namespace

int main() {
    std::cout
        << "GR3 walk_forward will request Walk (FSM 3), command "
        << kForwardVelocityMps
        << " m/s in the base-frame forward direction for "
        << kWalkDurationSeconds << " s.\n"
        << "Confirm the area is clear. Type YES to continue: ";
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
        options.enabled_endpoints = {
            sdk::Endpoint::FsmService,
            sdk::Endpoint::AuroraStateSubscriber,
            sdk::Endpoint::MotionCommandPublisher,
        };
        client.configure(options);
        client.start();

        if (!client.waitForEndpoints(std::chrono::seconds(5))) {
            std::cerr << "GR3 walk endpoints were not ready within 5 s.\n";
            client.stop();
            return interrupted ? 130 : 1;
        }
        if (interrupted) {
            client.stop();
            return 130;
        }

        auto result = client.requestFsmState(kGr3WalkState);
        if (!result.success()) {
            printResult("requestFsmState(Walk)", result);
            client.stop();
            return 1;
        }
        if (!client.waitForFsmState(
                kGr3WalkState, std::chrono::seconds(10))) {
            std::cerr << "GR3 did not enter Walk (FSM 3) within 10 s.\n";
            client.stop();
            return interrupted ? 130 : 1;
        }

        result = client.setVelocity(
            kForwardVelocityMps,
            0.0,
            0.0,
            kWalkDurationSeconds);
        if (!result.success()) {
            printResult("setVelocity", result);
            client.stop();
            return 1;
        }

        const auto deadline =
            std::chrono::steady_clock::now() +
            std::chrono::duration<double>(kWalkDurationSeconds);
        while (!interrupted &&
               std::chrono::steady_clock::now() < deadline) {
            std::this_thread::sleep_for(std::chrono::milliseconds(50));
        }

        client.stop();
        return interrupted ? 130 : 0;
    } catch (const std::exception& error) {
        std::cerr << "GR3 walk_forward failed: " << error.what() << "\n";
        stopAfterFailure(client);
        return interrupted ? 130 : 1;
    }
}
