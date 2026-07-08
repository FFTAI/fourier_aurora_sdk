#include <algorithm>
#include <chrono>
#include <cctype>
#include <cstdlib>
#include <cstdint>
#include <exception>
#include <iostream>
#include <limits>
#include <string>
#include <thread>
#include <vector>

#include "example_common.hpp"
#include "fourier_aurora_sdk/aurora_client.hpp"

namespace {

enum class OperationType {
    GetCurrentState,
    GetAvailableState,
    MonitorCurrentState,
    SetState,
};

struct Operation {
    OperationType type;
    uint32_t state_id = 0;
};

void printUsage(const char* program) {
    std::cout
        << "Usage: " << program << " domain_id user_id [arguments...]\n"
        << "Arguments:\n"
        << "  --get-state\n"
        << "  --get-available-state\n"
        << "  --monitor-state\n"
        << "  --set-state <int>\n";
}

bool parseStateId(const std::string& input, uint32_t& state_id, std::string& error) {
    if (input.empty() || !std::all_of(input.begin(), input.end(), [](unsigned char ch) {
            return std::isdigit(ch);
        })) {
        error = "state id must be an integer";
        return false;
    }

    unsigned long long parsed = 0;
    try {
        parsed = std::stoull(input);
    } catch (const std::exception&) {
        error = "state id is out of uint32_t range";
        return false;
    }
    if (parsed > std::numeric_limits<uint32_t>::max()) {
        error = "state id is out of uint32_t range";
        return false;
    }

    state_id = static_cast<uint32_t>(parsed);
    return true;
}

void printStateInfo(const fourier::aurora_sdk::StateInfo& state) {
    std::cout << "id: " << state.id << "\n"
              << "version: " << state.version << "\n"
              << "name: " << state.name << "\n"
              << "description: " << state.description << "\n"
              << "active: " << (state.active ? "true" : "false") << "\n";
}

void printAuroraStateHeader(const fourier::aurora_sdk::AuroraState& aurora_state) {
    example::printHeader(aurora_state.header);
}

void printSeparator() {
    std::cout << "------------------------------\n";
}

fourier::aurora_sdk::AuroraState waitForAuroraState(
    const fourier::aurora_sdk::AuroraClient& client) {
    std::exception_ptr last_error;
    for (int attempt = 0; attempt < 25; ++attempt) {
        try {
            return client.getAuroraState();
        } catch (...) {
            last_error = std::current_exception();
            std::this_thread::sleep_for(std::chrono::milliseconds(200));
        }
    }
    if (last_error) {
        std::rethrow_exception(last_error);
    }
    return client.getAuroraState();
}

void printCurrentState(const fourier::aurora_sdk::AuroraState& aurora_state) {
    printSeparator();
    printAuroraStateHeader(aurora_state);
    printStateInfo(aurora_state.current_state);
    std::cout << "switch_status: " << static_cast<int>(aurora_state.switch_status) << "\n";
}

void printAvailableStates(const fourier::aurora_sdk::AuroraState& aurora_state) {
    printAuroraStateHeader(aurora_state);
    for (size_t i = 0; i < aurora_state.available_states.size(); ++i) {
        printSeparator();
        printStateInfo(aurora_state.available_states[i]);
    }
}

}  // namespace

int main(int argc, char** argv) {
    if (argc < 4) {
        printUsage(argv[0]);
        return 1;
    }

    example::Config config;
    try {
        config.domain_id = static_cast<uint32_t>(std::stoul(argv[1]));
    } catch (const std::exception&) {
        std::cerr << "Invalid domain_id: enter an integer.\n";
        return 1;
    }
    config.user_id = argv[2];
    example::installStopSignalHandlers();

    std::vector<Operation> operations;
    for (int i = 3; i < argc; ++i) {
        const std::string arg = argv[i];
        if (arg == "--get-state") {
            operations.push_back({OperationType::GetCurrentState});
        } else if (arg == "--get-available-state") {
            operations.push_back({OperationType::GetAvailableState});
        } else if (arg == "--monitor-state") {
            operations.push_back({OperationType::MonitorCurrentState});
        } else if (arg == "--set-state") {
            if (i + 1 >= argc) {
                std::cerr << "--set-state requires an integer state id.\n";
                return 1;
            }
            uint32_t state_id = 0;
            std::string error;
            if (!parseStateId(argv[++i], state_id, error)) {
                std::cerr << "Invalid --set-state value: " << error << ".\n";
                return 1;
            }
            operations.push_back({OperationType::SetState, state_id});
        } else {
            std::cerr << "Unknown argument: " << arg << "\n";
            printUsage(argv[0]);
            return 1;
        }
    }

    const auto monitor_count = std::count_if(operations.begin(), operations.end(), [](const Operation& operation) {
        return operation.type == OperationType::MonitorCurrentState;
    });
    if (monitor_count > 0 && operations.size() > 1) {
        std::cerr << "--monitor-state cannot be combined with other arguments.\n";
        return 1;
    }

    fourier::aurora_sdk::AuroraClient* client = nullptr;
    try {
        client = &fourier::aurora_sdk::AuroraClient::getInstance(
            config.user_id,
            config.domain_id);
        std::this_thread::sleep_for(std::chrono::seconds(1));

        for (const auto& operation : operations) {
            switch (operation.type) {
                case OperationType::GetCurrentState:
                    printCurrentState(waitForAuroraState(*client));
                    break;
                case OperationType::GetAvailableState:
                    printAvailableStates(waitForAuroraState(*client));
                    break;
                case OperationType::MonitorCurrentState:
                    while (!example::stopRequested()) {
                        printCurrentState(waitForAuroraState(*client));
                        std::cout << "------------------------------\n";
                        std::this_thread::sleep_for(std::chrono::seconds(1));
                    }
                    break;
                case OperationType::SetState: {
                    const auto result = client->requestFsmState(operation.state_id);
                    std::cout << "requestFsmState(" << operation.state_id << ") ";
                    example::printServiceResult(result);
                    example::closeClientOrForceExit(client, result.success() ? 0 : 1);
                    return result.success() ? 0 : 1;
                }
            }
        }

        example::closeClientOrForceExit(client, 0);
    } catch (const std::exception& error) {
        std::cerr << "state_client failed: " << error.what() << "\n";
        example::closeClientOrForceExit(client, 1);
        return 1;
    }
}
