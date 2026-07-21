#include <chrono>
#include <exception>
#include <iostream>
#include <string>
#include <thread>
#include <type_traits>

#include "fourier_aurora_sdk/aurora_client.hpp"

namespace sdk = fourier::aurora_sdk;

namespace {

int runSurfaceCheck() {
    using fourier::aurora_sdk::AuroraClient;
    using fourier::aurora_sdk::ConnectionOptions;
    using fourier::aurora_sdk::MotionCommand;

    static_assert(!std::is_copy_constructible_v<AuroraClient>);
    static_assert(!std::is_move_constructible_v<AuroraClient>);

    ConnectionOptions options;
    MotionCommand command;
    command.lin_velocity = {0.1, 0.0, 0.0};
    command.duration = 0.2;

    std::cout << "fourier_aurora_sdk installed consumer: endpoints="
              << options.enabled_endpoints.size()
              << " duration=" << command.duration << "\n";
    return 0;
}

int runDdsLoopback() {
    using namespace std::chrono_literals;

    auto& client = sdk::AuroraClient::getInstance();
    try {
        client.stop();
        if (client.state() != sdk::ClientState::Stopped ||
            client.domainId() != 123 || client.userId() != "aurora_client") {
            std::cerr << "installed client defaults are incorrect\n";
            return 1;
        }

        client.setDomainId(214);
        client.setUserId("deb_loopback");
        auto options = client.config();
        options.namespace_name = "aurora_sdk_wheel_loopback";
        options.dds_mode = 0;
        options.is_ros_compatible = false;
        options.use_namespace = true;
        options.use_discovery_server = false;
        options.service_timeout_ms = 1000;
        options.enabled_endpoints = {
            sdk::Endpoint::AuroraStateSubscriber,
            sdk::Endpoint::MotionCommandPublisher,
            sdk::Endpoint::FsmService,
        };
        client.configure(options);

        client.start();
        if (client.state() != sdk::ClientState::Running ||
            !client.waitForEndpoints(5s)) {
            std::cerr << "installed client endpoints did not match\n";
            client.stop();
            return 1;
        }

        sdk::AuroraState state;
        sdk::OperationResult state_result;
        const auto state_deadline = std::chrono::steady_clock::now() + 3s;
        do {
            state_result = client.getAuroraState(state);
            if (state_result.success() && state.current_state.id == 13) {
                break;
            }
            std::this_thread::sleep_for(50ms);
        } while (std::chrono::steady_clock::now() < state_deadline);
        if (!state_result.success() || state.current_state.id != 13) {
            std::cerr << "installed client did not receive state: "
                      << state_result.code << " " << state_result.message
                      << "\n";
            client.stop();
            return 1;
        }

        const auto fsm_result = client.requestFsmState(13);
        if (fsm_result.code != 213 || fsm_result.message != "state=13") {
            std::cerr << "installed client received unexpected FSM result: "
                      << fsm_result.code << " " << fsm_result.message << "\n";
            client.stop();
            return 1;
        }

        const auto motion_result =
            client.setVelocity(0.1, 0.0, 0.0, 0.1);
        if (!motion_result.success()) {
            std::cerr << "installed client motion publish failed: "
                      << motion_result.code << " " << motion_result.message
                      << "\n";
            client.stop();
            return 1;
        }

        client.stop();
        std::cout << "installed C++ DDS loopback passed\n";
        return 0;
    } catch (const std::exception& error) {
        try {
            client.stop();
        } catch (...) {
        }
        std::cerr << "installed C++ DDS loopback failed: " << error.what()
                  << "\n";
        return 1;
    }
}

}  // namespace

int main(int argc, char** argv) {
    if (argc == 1) {
        return runSurfaceCheck();
    }
    if (argc == 2 && std::string(argv[1]) == "--dds-loopback") {
        return runDdsLoopback();
    }
    std::cerr << "Usage: " << argv[0] << " [--dds-loopback]\n";
    return 2;
}
