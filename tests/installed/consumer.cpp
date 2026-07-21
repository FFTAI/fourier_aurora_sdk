#include <iostream>
#include <type_traits>

#include "fourier_aurora_sdk/aurora_client.hpp"

int main() {
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
