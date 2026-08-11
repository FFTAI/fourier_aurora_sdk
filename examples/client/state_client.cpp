#include <algorithm>
#include <chrono>
#include <cstdint>
#include <exception>
#include <iostream>
#include <string>
#include <utility>
#include <vector>

#include "client_common.hpp"

namespace sdk = fourier::aurora_sdk;

namespace {

enum class OperationType {
    GetState,
    GetAvailableStates,
    MonitorState,
    RequestState,
};

struct Operation {
    OperationType type;
    uint32_t state_id = 0;
};

void printUsage(const char* program) {
    std::cerr
        << "Usage: " << program << " <domain_id> <source_id> <operation...>\n"
        << "Operations:\n"
        << "  --get-state\n"
        << "  --get-available-states\n"
        << "  --monitor-state\n"
        << "  --request-state <uint32>\n";
}

void printStateInfo(const sdk::StateInfo& state) {
    std::cout << "id: " << state.id << "\n"
              << "version: " << state.version << "\n"
              << "name: " << state.name << "\n"
              << "description: " << state.description << "\n"
              << "active: " << (state.active ? "true" : "false") << "\n";
}

void printCurrentState(const sdk::AuroraState& state) {
    std::cout << "------------------------------\n";
    client_example::printHeader(state.header);
    std::cout << "robot_type: " << state.robot_info.robot_type << "\n"
              << "hardware_type: " << state.robot_info.hardware_type << "\n"
              << "end_effector_type: "
              << state.robot_info.end_effector_type << "\n"
              << "switch_status: " << static_cast<int>(state.switch_status)
              << "\ncurrent_state:\n";
    printStateInfo(state.current_state);
}

void printAvailableStates(const sdk::AuroraState& state) {
    client_example::printHeader(state.header);
    std::cout << "available_states: " << state.available_states.size() << "\n";
    for (const auto& available : state.available_states) {
        std::cout << "------------------------------\n";
        printStateInfo(available);
    }
}

int monitorState(sdk::AuroraClient& client) {
    client_example::FailureSnapshot failure;
    while (!client_example::stopRequested()) {
        sdk::AuroraState state;
        const auto result = client.getAuroraState(state);
        if (result.success()) {
            failure.clear();
            printCurrentState(state);
        } else if (failure.changed(result)) {
            client_example::printResult("getAuroraState", result);
        }
        client_example::sleepInterruptibly(std::chrono::milliseconds(500));
    }
    return client_example::kInterrupted;
}

}  // namespace

int main(int argc, char** argv) {
    if (argc < 4) {
        printUsage(argv[0]);
        return client_example::kUsageError;
    }

    uint32_t domain_id = 0;
    if (!client_example::parseUint32(argv[1], domain_id) ||
        std::string(argv[2]).empty()) {
        std::cerr << "domain_id and source_id must be uint32 values.\n";
        return client_example::kUsageError;
    }

    std::vector<Operation> operations;
    for (int index = 3; index < argc; ++index) {
        const std::string argument = argv[index];
        if (argument == "--get-state") {
            operations.push_back({OperationType::GetState});
        } else if (argument == "--get-available-states") {
            operations.push_back({OperationType::GetAvailableStates});
        } else if (argument == "--monitor-state") {
            operations.push_back({OperationType::MonitorState});
        } else if (argument == "--request-state") {
            if (++index >= argc) {
                std::cerr << "--request-state requires a state ID.\n";
                return client_example::kUsageError;
            }
            uint32_t state_id = 0;
            if (!client_example::parseUint32(argv[index], state_id)) {
                std::cerr << "state ID must be uint32.\n";
                return client_example::kUsageError;
            }
            operations.push_back({OperationType::RequestState, state_id});
        } else {
            std::cerr << "Unknown operation: " << argument << "\n";
            printUsage(argv[0]);
            return client_example::kUsageError;
        }
    }

    const auto monitor_count = std::count_if(
        operations.begin(),
        operations.end(),
        [](const Operation& operation) {
            return operation.type == OperationType::MonitorState;
        });
    if (operations.empty() ||
        (monitor_count != 0 && operations.size() != 1)) {
        std::cerr << "--monitor-state must be the only operation.\n";
        return client_example::kUsageError;
    }

    client_example::installSignalHandlers();
    auto& client = sdk::AuroraClient::getInstance();
    try {
        const bool reads_state = std::any_of(
            operations.begin(),
            operations.end(),
            [](const Operation& operation) {
                return operation.type != OperationType::RequestState;
            });
        const bool requests_state = std::any_of(
            operations.begin(),
            operations.end(),
            [](const Operation& operation) {
                return operation.type == OperationType::RequestState;
            });
        std::vector<sdk::Endpoint> endpoints;
        if (reads_state) {
            endpoints.push_back(sdk::Endpoint::AuroraStateSubscriber);
        }
        if (requests_state) {
            endpoints.push_back(sdk::Endpoint::FsmService);
        }
        client_example::configureClient(
            client,
            domain_id,
            argv[2],
            std::move(endpoints));
        client.start();
        if (!client.waitForEndpoints(std::chrono::seconds(5))) {
            std::cerr << "Timed out waiting for required endpoints.\n";
            return client_example::stopClient(
                client, client_example::kRuntimeFailure);
        }

        int exit_code = client_example::kSuccess;
        if (operations.front().type == OperationType::MonitorState) {
            exit_code = monitorState(client);
        } else {
            for (const auto& operation : operations) {
                if (client_example::stopRequested()) {
                    exit_code = client_example::kInterrupted;
                    break;
                }
                if (operation.type == OperationType::RequestState) {
                    const auto result =
                        client.requestFsmState(operation.state_id);
                    client_example::printResult(
                        "requestFsmState", result, std::cout);
                    if (!result.success()) {
                        exit_code = client_example::kRuntimeFailure;
                        break;
                    }
                    continue;
                }

                sdk::AuroraState state;
                const auto result = client.getAuroraState(state);
                if (!result.success()) {
                    client_example::printResult("getAuroraState", result);
                    exit_code = client_example::kRuntimeFailure;
                    break;
                }
                if (operation.type == OperationType::GetState) {
                    printCurrentState(state);
                } else {
                    printAvailableStates(state);
                }
            }
        }
        return client_example::stopClient(client, exit_code);
    } catch (const std::exception& error) {
        std::cerr << "state_client failed: " << error.what() << "\n";
        return client_example::stopClient(
            client, client_example::kRuntimeFailure);
    }
}
