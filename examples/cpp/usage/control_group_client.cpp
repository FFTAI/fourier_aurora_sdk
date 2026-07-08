#include <algorithm>
#include <chrono>
#include <cstdint>
#include <exception>
#include <iostream>
#include <sstream>
#include <string>
#include <thread>
#include <vector>

#include "example_common.hpp"
#include "fourier_aurora_sdk/aurora_client.hpp"

namespace {

enum class Operation {
    GetState,
    MonitorState,
    SetPosition,
};

struct PositionTarget {
    std::string group_name;
    std::vector<double> position;
};

struct ParsedArgs {
    Operation operation = Operation::GetState;
    bool has_operation = false;
    std::vector<PositionTarget> position_targets;
    double duration = 5.0;
    double rate = 100.0;
};

void printUsage(const char* program) {
    std::cout
        << "Usage: " << program << " domain_id user_id [arguments...]\n"
        << "Arguments:\n"
        << "  --get-state\n"
        << "  --monitor-state\n"
        << "  --position <group_name> <target...> | [target0,target1,...]\n"
        << "  --duration <seconds>   default: 5.0\n"
        << "  --rate <hz>            default: 100\n";
}

std::string joinBracketTokens(int argc, char** argv, int& index) {
    std::string value = argv[index];
    while (value.find(']') == std::string::npos && index + 1 < argc) {
        value += argv[++index];
    }
    return value;
}

std::vector<std::string> splitList(std::string value) {
    value.erase(std::remove_if(value.begin(), value.end(), [](unsigned char ch) {
        return std::isspace(ch) || ch == '[' || ch == ']';
    }), value.end());

    std::vector<std::string> parts;
    std::stringstream stream(value);
    std::string part;
    while (std::getline(stream, part, ',')) {
        if (!part.empty()) {
            parts.push_back(part);
        }
    }
    return parts;
}

bool parseDouble(const std::string& input, double& value) {
    size_t consumed = 0;
    try {
        value = std::stod(input, &consumed);
    } catch (const std::exception&) {
        return false;
    }
    return consumed == input.size();
}

bool parseDoubleList(
    int argc,
    char** argv,
    int& index,
    std::vector<double>& values,
    std::string& error) {
    values.clear();
    if (index + 1 >= argc) {
        error = "missing vector value";
        return false;
    }

    std::vector<std::string> parts;
    const std::string first = argv[++index];
    if (!first.empty() && first.front() == '[') {
        auto list = first;
        if (list.find(']') == std::string::npos) {
            list = joinBracketTokens(argc, argv, index);
        }
        parts = splitList(list);
    } else {
        parts.push_back(first);
        while (index + 1 < argc) {
            const std::string next = argv[index + 1];
            if (next.rfind("--", 0) == 0) {
                break;
            }
            parts.push_back(argv[++index]);
        }
    }

    if (parts.empty()) {
        error = "expected at least one value";
        return false;
    }

    values.reserve(parts.size());
    for (const auto& part : parts) {
        double value = 0.0;
        if (!parseDouble(part, value)) {
            error = "invalid numeric value: " + part;
            return false;
        }
        values.push_back(value);
    }
    return true;
}

fourier::aurora_sdk::ControlGroupState waitForControlGroupState(
    const fourier::aurora_sdk::AuroraClient& client) {
    std::exception_ptr last_error;
    for (int attempt = 0; attempt < 25; ++attempt) {
        try {
            return client.getControlGroupState();
        } catch (...) {
            last_error = std::current_exception();
            std::this_thread::sleep_for(std::chrono::milliseconds(200));
        }
    }
    if (last_error) {
        std::rethrow_exception(last_error);
    }
    return client.getControlGroupState();
}

const fourier::aurora_sdk::ControlGroupStateEntry* findCommandGroup(
    const fourier::aurora_sdk::ControlGroupState& state,
    const std::string& group_name) {
    const auto it = std::find_if(
        state.joint_cmd.begin(),
        state.joint_cmd.end(),
        [&](const fourier::aurora_sdk::ControlGroupStateEntry& entry) {
            return entry.group_name == group_name;
        });
    if (it == state.joint_cmd.end()) {
        return nullptr;
    }
    return &*it;
}

void printJointGroup(
    const fourier::aurora_sdk::ControlGroupStateEntry& joint_state,
    const fourier::aurora_sdk::ControlGroupStateEntry* joint_cmd) {
    std::cout << "group_name: " << joint_state.group_name << "\n";

    std::cout << "position: ";
    example::printVector(joint_state.joint_position);
    std::cout << "\nvelocity: ";
    example::printVector(joint_state.joint_velocity);
    std::cout << "\neffort: ";
    example::printVector(joint_state.joint_effort);

    std::cout << "\nposition_cmd: ";
    if (joint_cmd != nullptr) {
        example::printVector(joint_cmd->joint_position);
    } else {
        example::printVector({});
    }
    std::cout << "\nvelocity_cmd: ";
    if (joint_cmd != nullptr) {
        example::printVector(joint_cmd->joint_velocity);
    } else {
        example::printVector({});
    }
    std::cout << "\neffort_cmd: ";
    if (joint_cmd != nullptr) {
        example::printVector(joint_cmd->joint_effort);
    } else {
        example::printVector({});
    }
    std::cout << "\n";
}

void printControlGroupState(const fourier::aurora_sdk::ControlGroupState& state) {
    std::cout << "------------------------------\n";
    for (const auto& joint_state : state.joint_state) {
        printJointGroup(joint_state, findCommandGroup(state, joint_state.group_name));
        std::cout << "------------------------------\n";
    }
}

void setOperation(ParsedArgs& parsed, Operation operation, const char* option) {
    if (parsed.has_operation && parsed.operation != operation) {
        throw std::runtime_error(std::string(option) + " cannot be combined with other operations");
    }
    parsed.operation = operation;
    parsed.has_operation = true;
}

ParsedArgs parseArgs(int argc, char** argv) {
    ParsedArgs parsed;
    for (int i = 3; i < argc; ++i) {
        const std::string arg = argv[i];
        if (arg == "--get-state") {
            setOperation(parsed, Operation::GetState, "--get-state");
        } else if (arg == "--monitor-state") {
            setOperation(parsed, Operation::MonitorState, "--monitor-state");
        } else if (arg == "--position") {
            setOperation(parsed, Operation::SetPosition, "--position");
            if (i + 1 >= argc) {
                throw std::runtime_error("--position requires a group name and target position");
            }
            PositionTarget target;
            target.group_name = argv[++i];
            std::string error;
            if (!parseDoubleList(argc, argv, i, target.position, error)) {
                throw std::runtime_error("--position " + target.group_name + ": " + error);
            }
            parsed.position_targets.push_back(std::move(target));
        } else if (arg == "--duration") {
            if (i + 1 >= argc || !parseDouble(argv[++i], parsed.duration) || parsed.duration <= 0.0) {
                throw std::runtime_error("--duration requires a positive numeric value");
            }
        } else if (arg == "--rate") {
            if (i + 1 >= argc || !parseDouble(argv[++i], parsed.rate) || parsed.rate <= 0.0) {
                throw std::runtime_error("--rate requires a positive numeric value");
            }
        } else if (arg == "--set_position") {
            throw std::runtime_error("use --position instead of --set_position");
        } else {
            throw std::runtime_error("Unknown argument: " + arg);
        }
    }

    if (!parsed.has_operation) {
        throw std::runtime_error("Missing operation");
    }
    if (parsed.operation == Operation::SetPosition && parsed.position_targets.empty()) {
        throw std::runtime_error("--position requires at least one group target");
    }
    return parsed;
}

const fourier::aurora_sdk::ControlGroupStateEntry& findStateGroup(
    const fourier::aurora_sdk::ControlGroupState& state,
    const std::string& group_name) {
    const auto it = std::find_if(
        state.joint_state.begin(),
        state.joint_state.end(),
        [&](const fourier::aurora_sdk::ControlGroupStateEntry& entry) {
            return entry.group_name == group_name;
        });
    if (it == state.joint_state.end()) {
        throw std::runtime_error("joint state group not found: " + group_name);
    }
    return *it;
}

std::vector<double> getStartPosition(
    const fourier::aurora_sdk::ControlGroupState& state,
    const std::string& group_name) {
    const auto* cmd_group = findCommandGroup(state, group_name);
    if (cmd_group != nullptr && !cmd_group->joint_position.empty()) {
        return cmd_group->joint_position;
    }
    return findStateGroup(state, group_name).joint_position;
}

fourier::aurora_sdk::JointCommand makeJointCommand(
    const std::string& group_name,
    const std::vector<double>& position) {
    fourier::aurora_sdk::JointCommand command;
    command.group_name = group_name;
    command.control_type = fourier::aurora_sdk::JointControlType::Joint;
    command.motor_mode = fourier::aurora_sdk::MotorMode::Pd;
    command.position = position;
    command.velocity.assign(position.size(), 0.0);
    command.effort.assign(position.size(), 0.0);
    return command;
}

void runPositionInterpolation(
    fourier::aurora_sdk::AuroraClient& client,
    const std::vector<PositionTarget>& targets,
    double duration,
    double rate) {
    const auto state = waitForControlGroupState(client);
    std::vector<std::vector<double>> starts;
    starts.reserve(targets.size());

    for (const auto& target : targets) {
        auto start = getStartPosition(state, target.group_name);
        if (start.size() != target.position.size()) {
            throw std::runtime_error(
                "target length for " + target.group_name + " is " +
                std::to_string(target.position.size()) + ", expected " +
                std::to_string(start.size()));
        }
        starts.push_back(std::move(start));
    }

    const int steps = std::max(1, static_cast<int>(duration * rate));
    const auto sleep_dt = std::chrono::duration_cast<std::chrono::steady_clock::duration>(
        std::chrono::duration<double>(1.0 / rate));

    for (int step = 1; step <= steps && !example::stopRequested(); ++step) {
        const double alpha = static_cast<double>(step) / static_cast<double>(steps);
        fourier::aurora_sdk::ControlGroupCommand command;
        command.joint_cmd.reserve(targets.size());

        for (size_t target_index = 0; target_index < targets.size(); ++target_index) {
            std::vector<double> position = starts[target_index];
            for (size_t joint_index = 0; joint_index < position.size(); ++joint_index) {
                position[joint_index] =
                    starts[target_index][joint_index] +
                    alpha * (targets[target_index].position[joint_index] -
                             starts[target_index][joint_index]);
            }
            command.joint_cmd.push_back(
                makeJointCommand(targets[target_index].group_name, position));
        }

        client.publishControlGroupCommand(command);
        std::this_thread::sleep_for(sleep_dt);
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

    ParsedArgs parsed;
    try {
        parsed = parseArgs(argc, argv);
    } catch (const std::exception& error) {
        std::cerr << error.what() << "\n";
        printUsage(argv[0]);
        return 1;
    }

    fourier::aurora_sdk::AuroraClient* client = nullptr;
    try {
        client = &fourier::aurora_sdk::AuroraClient::getInstance(config.user_id, config.domain_id);
        std::this_thread::sleep_for(std::chrono::seconds(1));

        if (parsed.operation == Operation::MonitorState) {
            while (!example::stopRequested()) {
                printControlGroupState(waitForControlGroupState(*client));
                std::this_thread::sleep_for(std::chrono::seconds(1));
            }
        } else if (parsed.operation == Operation::SetPosition) {
            runPositionInterpolation(
                *client,
                parsed.position_targets,
                parsed.duration,
                parsed.rate);
        } else {
            printControlGroupState(waitForControlGroupState(*client));
        }

        example::closeClientOrForceExit(client, 0);
    } catch (const std::exception& error) {
        std::cerr << "control_group_client failed: " << error.what() << "\n";
        example::closeClientOrForceExit(client, 1);
        return 1;
    }
}
