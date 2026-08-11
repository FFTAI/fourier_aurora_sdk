#include <algorithm>
#include <chrono>
#include <cmath>
#include <exception>
#include <iostream>
#include <string>
#include <thread>
#include <utility>
#include <vector>

#include "client_common.hpp"

namespace sdk = fourier::aurora_sdk;

namespace {

enum class Operation {
    None,
    GetState,
    MonitorState,
    SetPosition,
};

struct PositionTarget {
    std::string group_name;
    std::vector<double> position;
};

struct Arguments {
    Operation operation = Operation::None;
    std::vector<PositionTarget> targets;
    double duration = 5.0;
    double rate_hz = 100.0;
};

void printUsage(const char* program) {
    std::cerr
        << "Usage: " << program << " <domain_id> <source_id> <operation>\n"
        << "Operations:\n"
        << "  --get-state\n"
        << "  --monitor-state\n"
        << "  --position <group_name> <target0> [target1 ...]\n"
        << "Options for --position:\n"
        << "  --duration <seconds>  default: 5\n"
        << "  --rate <hz>           default: 100\n";
}

bool selectOperation(
    Arguments& parsed,
    Operation operation,
    std::string& error) {
    if (parsed.operation != Operation::None &&
        parsed.operation != operation) {
        error = "read, monitor, and position operations cannot be combined";
        return false;
    }
    parsed.operation = operation;
    return true;
}

bool parseArguments(
    int argc,
    char** argv,
    Arguments& parsed,
    std::string& error) {
    for (int index = 3; index < argc; ++index) {
        const std::string argument = argv[index];
        if (argument == "--get-state") {
            if (!selectOperation(parsed, Operation::GetState, error)) {
                return false;
            }
        } else if (argument == "--monitor-state") {
            if (!selectOperation(parsed, Operation::MonitorState, error)) {
                return false;
            }
        } else if (argument == "--position") {
            if (!selectOperation(parsed, Operation::SetPosition, error) ||
                ++index >= argc) {
                error = "--position requires a group name and target values";
                return false;
            }
            PositionTarget target;
            target.group_name = argv[index];
            while (index + 1 < argc &&
                   std::string(argv[index + 1]).rfind("--", 0) != 0) {
                double value = 0.0;
                if (!client_example::parseDouble(argv[++index], value)) {
                    error = "position targets must be finite numbers";
                    return false;
                }
                target.position.push_back(value);
            }
            if (target.group_name.empty() || target.position.empty()) {
                error = "--position requires a group name and target values";
                return false;
            }
            parsed.targets.push_back(std::move(target));
        } else if (argument == "--duration") {
            if (++index >= argc ||
                !client_example::parseDouble(
                    argv[index], parsed.duration) ||
                parsed.duration <= 0.0) {
                error = "--duration requires a positive finite value";
                return false;
            }
        } else if (argument == "--rate") {
            if (++index >= argc ||
                !client_example::parseDouble(
                    argv[index], parsed.rate_hz) ||
                parsed.rate_hz <= 0.0) {
                error = "--rate requires a positive finite value";
                return false;
            }
        } else {
            error = "unknown operation: " + argument;
            return false;
        }
    }

    if (parsed.operation == Operation::None) {
        error = "an operation is required";
        return false;
    }
    if (parsed.operation != Operation::SetPosition &&
        (parsed.duration != 5.0 || parsed.rate_hz != 100.0)) {
        error = "--duration and --rate require --position";
        return false;
    }
    return true;
}

const sdk::ControlGroupStateEntry* findGroup(
    const std::vector<sdk::ControlGroupStateEntry>& groups,
    const std::string& name) {
    const auto found = std::find_if(
        groups.begin(),
        groups.end(),
        [&](const sdk::ControlGroupStateEntry& group) {
            return group.group_name == name;
        });
    return found == groups.end() ? nullptr : &*found;
}

void printGroup(
    const sdk::ControlGroupStateEntry& state,
    const sdk::ControlGroupStateEntry* command) {
    std::cout << "group_name: " << state.group_name << "\nposition: ";
    client_example::printRange(state.joint_position);
    std::cout << "\nvelocity: ";
    client_example::printRange(state.joint_velocity);
    std::cout << "\neffort: ";
    client_example::printRange(state.joint_effort);
    std::cout << "\nposition_cmd: ";
    if (command) {
        client_example::printRange(command->joint_position);
    } else {
        client_example::printRange(std::vector<double>{});
    }
    std::cout << "\nvelocity_cmd: ";
    if (command) {
        client_example::printRange(command->joint_velocity);
    } else {
        client_example::printRange(std::vector<double>{});
    }
    std::cout << "\neffort_cmd: ";
    if (command) {
        client_example::printRange(command->joint_effort);
    } else {
        client_example::printRange(std::vector<double>{});
    }
    std::cout << "\n";
}

void printCartesian(const sdk::CartesianStateEntry& state) {
    std::cout << "link_name: " << state.link_name << "\npose: ";
    client_example::printRange(state.pose);
    std::cout << "\ntwist: ";
    client_example::printRange(state.twist);
    std::cout << "\nwrench: ";
    client_example::printRange(state.wrench);
    std::cout << "\n";
}

void printControlGroupState(const sdk::ControlGroupState& state) {
    std::cout << "==============================\n";
    client_example::printHeader(state.header);
    std::cout << "joint_state:\n";
    for (const auto& group : state.joint_state) {
        std::cout << "------------------------------\n";
        printGroup(group, findGroup(state.joint_cmd, group.group_name));
    }
    std::cout << "cartesian_state:\n";
    for (const auto& link : state.cartesian_state) {
        std::cout << "------------------------------\n";
        printCartesian(link);
    }
    std::cout << "cartesian_cmd:\n";
    for (const auto& link : state.cartesian_cmd) {
        std::cout << "------------------------------\n";
        printCartesian(link);
    }
}

sdk::JointCommand makeJointCommand(
    const std::string& group_name,
    std::vector<double> position) {
    sdk::JointCommand command;
    command.group_name = group_name;
    command.control_type = sdk::JointControlType::Joint;
    command.motor_mode = sdk::MotorMode::Pd;
    command.velocity.assign(position.size(), 0.0);
    command.effort.assign(position.size(), 0.0);
    command.position = std::move(position);
    return command;
}

int setPositions(
    sdk::AuroraClient& client,
    const Arguments& parsed) {
    sdk::ControlGroupState state;
    const auto state_result = client.getControlGroupState(state);
    if (!state_result.success()) {
        client_example::printResult(
            "getControlGroupState", state_result);
        return client_example::kRuntimeFailure;
    }

    std::vector<std::vector<double>> starts;
    starts.reserve(parsed.targets.size());
    for (const auto& target : parsed.targets) {
        const auto* group = findGroup(state.joint_state, target.group_name);
        if (!group) {
            std::cerr << "group not found: " << target.group_name << "\n";
            return client_example::kRuntimeFailure;
        }
        if (group->joint_position.size() != target.position.size()) {
            std::cerr << "target length for " << target.group_name
                      << " is " << target.position.size()
                      << ", expected " << group->joint_position.size()
                      << "\n";
            return client_example::kRuntimeFailure;
        }
        starts.push_back(group->joint_position);
    }

    const auto begin = std::chrono::steady_clock::now();
    const auto duration = std::chrono::duration<double>(parsed.duration);
    const auto period =
        std::chrono::duration<double>(1.0 / parsed.rate_hz);
    std::size_t step = 0;
    for (;;) {
        if (client_example::stopRequested()) {
            return client_example::kInterrupted;
        }
        const auto elapsed = std::chrono::steady_clock::now() - begin;
        const double alpha =
            std::min(1.0, std::chrono::duration<double>(elapsed).count() /
                              parsed.duration);

        sdk::ControlGroupCommand command;
        command.joint_cmd.reserve(parsed.targets.size());
        for (std::size_t target_index = 0;
             target_index < parsed.targets.size();
             ++target_index) {
            auto position = starts[target_index];
            for (std::size_t joint = 0; joint < position.size(); ++joint) {
                position[joint] =
                    starts[target_index][joint] +
                    alpha *
                        (parsed.targets[target_index].position[joint] -
                         starts[target_index][joint]);
            }
            command.joint_cmd.push_back(makeJointCommand(
                parsed.targets[target_index].group_name,
                std::move(position)));
        }

        const auto result =
            client.publishControlGroupCommand(command);
        if (!result.success()) {
            client_example::printResult(
                "publishControlGroupCommand", result);
            return client_example::kRuntimeFailure;
        }
        if (elapsed >= duration) {
            return client_example::kSuccess;
        }
        ++step;
        std::this_thread::sleep_until(
            begin + std::chrono::duration_cast<
                        std::chrono::steady_clock::duration>(
                        period * static_cast<double>(step)));
    }
}

int monitorState(sdk::AuroraClient& client) {
    client_example::FailureSnapshot failure;
    while (!client_example::stopRequested()) {
        sdk::ControlGroupState state;
        const auto result = client.getControlGroupState(state);
        if (result.success()) {
            failure.clear();
            printControlGroupState(state);
        } else if (failure.changed(result)) {
            client_example::printResult(
                "getControlGroupState", result);
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

    Arguments parsed;
    std::string parse_error;
    if (!parseArguments(argc, argv, parsed, parse_error)) {
        std::cerr << parse_error << "\n";
        printUsage(argv[0]);
        return client_example::kUsageError;
    }

    client_example::installSignalHandlers();
    auto& client = sdk::AuroraClient::getInstance();
    try {
        std::vector<sdk::Endpoint> endpoints{
            sdk::Endpoint::ControlGroupStateSubscriber};
        if (parsed.operation == Operation::SetPosition) {
            endpoints.push_back(sdk::Endpoint::ManageLeaseService);
            endpoints.push_back(
                sdk::Endpoint::ControlGroupCommandPublisher);
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
        if (parsed.operation == Operation::SetPosition) {
            std::vector<sdk::AuroraResource> resources;
            const auto convert = [](const std::string& group) {
                if (group == "left_leg") return sdk::AuroraResource::LeftLeg;
                if (group == "right_leg") return sdk::AuroraResource::RightLeg;
                if (group == "waist") return sdk::AuroraResource::Waist;
                if (group == "head") return sdk::AuroraResource::Head;
                if (group == "left_manipulator") return sdk::AuroraResource::LeftManipulator;
                if (group == "right_manipulator") return sdk::AuroraResource::RightManipulator;
                if (group == "left_end_effector") return sdk::AuroraResource::LeftEndEffector;
                if (group == "right_end_effector") return sdk::AuroraResource::RightEndEffector;
                throw sdk::InvalidArgumentError("unsupported control group: " + group);
            };
            for (const auto& target : parsed.targets) {
                const auto resource = convert(target.group_name);
                if (std::find(resources.begin(), resources.end(), resource) == resources.end()) {
                    resources.push_back(resource);
                }
            }
            const auto lease = client.registerLease(resources);
            if (!lease.success()) {
                client_example::printResult("registerLease", lease.operation);
                return client_example::stopClient(client, client_example::kRuntimeFailure);
            }
        }

        int exit_code = client_example::kSuccess;
        if (client_example::stopRequested()) {
            exit_code = client_example::kInterrupted;
        } else if (parsed.operation == Operation::MonitorState) {
            exit_code = monitorState(client);
        } else if (parsed.operation == Operation::SetPosition) {
            exit_code = setPositions(client, parsed);
        } else {
            sdk::ControlGroupState state;
            const auto result = client.getControlGroupState(state);
            if (!result.success()) {
                client_example::printResult(
                    "getControlGroupState", result);
                exit_code = client_example::kRuntimeFailure;
            } else {
                printControlGroupState(state);
            }
        }
        return client_example::stopClient(client, exit_code);
    } catch (const std::exception& error) {
        std::cerr << "control_group_client failed: "
                  << error.what() << "\n";
        return client_example::stopClient(
            client, client_example::kRuntimeFailure);
    }
}
