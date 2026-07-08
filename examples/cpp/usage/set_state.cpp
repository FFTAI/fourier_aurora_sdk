#include <chrono>
#include <algorithm>
#include <cctype>
#include <cstdlib>
#include <cstdint>
#include <exception>
#include <iostream>
#include <limits>
#include <string>
#include <thread>

#include "example_common.hpp"
#include "fourier_aurora_sdk/aurora_client.hpp"

int main(int argc, char** argv) {
    example::printUsage(argv[0]);
    const auto config = example::parseConfig(argc, argv, "cpp_set_state");

    try {
        fourier::aurora_sdk::AuroraClient* client = nullptr;

        while (true) {
            std::cout << "FSM state id> ";
            std::string input;
            if (!std::getline(std::cin, input)) {
                break;
            }
            if (input.empty() || !std::all_of(input.begin(), input.end(), [](unsigned char ch) {
                    return std::isdigit(ch);
                })) {
                std::cerr << "Invalid state id: enter an integer.\n";
                continue;
            }

            unsigned long long parsed_state_id = 0;
            try {
                parsed_state_id = std::stoull(input);
            } catch (const std::exception&) {
                std::cerr << "Invalid state id: integer is out of uint32_t range.\n";
                continue;
            }
            if (parsed_state_id > std::numeric_limits<uint32_t>::max()) {
                std::cerr << "Invalid state id: integer is out of uint32_t range.\n";
                continue;
            }
            const auto state_id = static_cast<uint32_t>(parsed_state_id);

            if (client == nullptr) {
                client = &fourier::aurora_sdk::AuroraClient::getInstance(
                    config.user_id,
                    config.domain_id);
                std::this_thread::sleep_for(std::chrono::seconds(1));
            }

            const auto result = client->requestFsmState(state_id);
            std::cout << "requestFsmState(" << state_id << ") ";
            example::printServiceResult(result);
            example::closeClientOrForceExit(client, result.success() ? 0 : 1);
            return result.success() ? 0 : 1;
        }

        example::closeClientOrForceExit(client, 0);
    } catch (const std::exception& error) {
        std::cerr << "set_state failed: " << error.what() << "\n";
        return 1;
    }
}
