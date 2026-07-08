#include <iostream>

#include "fourier_aurora_sdk/connection_options.hpp"
#include "fourier_aurora_sdk/types.hpp"

int main() {
    fourier::aurora_sdk::validateUserId("installed_smoke");

    fourier::aurora_sdk::MotionCommand command;
    command.lin_velocity = {0.1, 0.0, 0.0};
    command.duration = 0.2;

    std::cout << "fourier_aurora_sdk installed smoke ok: duration="
              << command.duration << std::endl;
    return 0;
}
