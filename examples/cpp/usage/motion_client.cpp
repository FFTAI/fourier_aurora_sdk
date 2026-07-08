#include <algorithm>
#include <array>
#include <chrono>
#include <cctype>
#include <cstdlib>
#include <cstdint>
#include <exception>
#include <iostream>
#include <limits>
#include <sstream>
#include <string>
#include <thread>
#include <vector>

#include "example_common.hpp"
#include "fourier_aurora_sdk/aurora_client.hpp"

namespace {

struct ParsedArgs {
    bool get_motion_state = false;
    bool monitor_motion_state = false;
    bool has_setter = false;
    fourier::aurora_sdk::MotionCommand command;
};

void printUsage(const char* program) {
    std::cout
        << "Usage: " << program << " domain_id user_id [arguments...]\n"
        << "Arguments:\n"
        << "  --get-state\n"
        << "  --monitor-state\n"
        << "  --position <x y z> | [x,y,z]\n"
        << "  --lin-velocity <vx vy vz> | [vx,vy,vz]\n"
        << "  --attitude <roll pitch yaw> | [roll,pitch,yaw]\n"
        << "  --ang-velocity <wx wy wz> | [wx,wy,wz]\n"
        << "  --delta-height <delta_height>\n"
        << "  --gait-type <uint8>\n"
        << "  --skills <s0 s1 s2 s3 s4 s5 s6 s7> | [s0,s1,s2,s3,s4,s5,s6,s7]\n"
        << "  --duration <duration>\n";
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

bool parseUint8(const std::string& input, uint8_t& value) {
    size_t consumed = 0;
    unsigned long parsed = 0;
    try {
        parsed = std::stoul(input, &consumed);
    } catch (const std::exception&) {
        return false;
    }
    if (consumed != input.size() || parsed > std::numeric_limits<uint8_t>::max()) {
        return false;
    }
    value = static_cast<uint8_t>(parsed);
    return true;
}

bool parseDoubleArray(
    int argc,
    char** argv,
    int& index,
    size_t count,
    std::vector<double>& values,
    std::string& error) {
    values.clear();
    if (index + 1 >= argc) {
        error = "missing vector value";
        return false;
    }

    const std::string first = argv[++index];
    std::vector<std::string> parts;
    if (!first.empty() && first.front() == '[') {
        auto list = first;
        if (list.find(']') == std::string::npos) {
            list = joinBracketTokens(argc, argv, index);
        }
        parts = splitList(list);
    } else {
        parts.push_back(first);
        while (parts.size() < count && index + 1 < argc) {
            parts.push_back(argv[++index]);
        }
    }

    if (parts.size() != count) {
        error = "expected " + std::to_string(count) + " values";
        return false;
    }
    values.reserve(count);
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

bool parseUint8Array(
    int argc,
    char** argv,
    int& index,
    size_t count,
    std::array<uint8_t, 8>& values,
    std::string& error) {
    if (count != values.size() || index + 1 >= argc) {
        error = "missing skills value";
        return false;
    }

    const std::string first = argv[++index];
    std::vector<std::string> parts;
    if (!first.empty() && first.front() == '[') {
        auto list = first;
        if (list.find(']') == std::string::npos) {
            list = joinBracketTokens(argc, argv, index);
        }
        parts = splitList(list);
    } else {
        parts.push_back(first);
        while (parts.size() < count && index + 1 < argc) {
            parts.push_back(argv[++index]);
        }
    }

    if (parts.size() != count) {
        error = "expected " + std::to_string(count) + " skill values";
        return false;
    }
    for (size_t i = 0; i < count; ++i) {
        uint8_t value = 0;
        if (!parseUint8(parts[i], value)) {
            error = "invalid uint8 skill value: " + parts[i];
            return false;
        }
        values[i] = value;
    }
    return true;
}

void printArray3(const std::array<double, 3>& values) {
    std::cout << "[" << values[0] << ", " << values[1] << ", " << values[2] << "]";
}

void printMotionState(const fourier::aurora_sdk::MotionState& state) {
    example::printHeader(state.header);
    std::cout << "position: ";
    printArray3(state.position);
    std::cout << "\nposition_cmd: ";
    printArray3(state.position_cmd);
    std::cout << "\nlin_velocity: ";
    printArray3(state.lin_velocity);
    std::cout << "\nlin_velocity_cmd: ";
    printArray3(state.lin_velocity_cmd);
    std::cout << "\nattitude: ";
    printArray3(state.attitude);
    std::cout << "\nattitude_cmd: ";
    printArray3(state.attitude_cmd);
    std::cout << "\nang_velocity: ";
    printArray3(state.ang_velocity);
    std::cout << "\nang_velocity_cmd: ";
    printArray3(state.ang_velocity_cmd);
    std::cout << "\ndelta_height: " << state.delta_height
              << "\ndelta_height_cmd: " << state.delta_height_cmd
              << "\nprogress: " << static_cast<int>(state.progress)
              << "\nstable_level: " << static_cast<int>(state.stable_level)
              << "\n";
}

fourier::aurora_sdk::MotionState waitForMotionState(
    const fourier::aurora_sdk::AuroraClient& client) {
    std::exception_ptr last_error;
    for (int attempt = 0; attempt < 25; ++attempt) {
        try {
            return client.getMotionState();
        } catch (...) {
            last_error = std::current_exception();
            std::this_thread::sleep_for(std::chrono::milliseconds(200));
        }
    }
    if (last_error) {
        std::rethrow_exception(last_error);
    }
    return client.getMotionState();
}

bool parseArgs(int argc, char** argv, ParsedArgs& parsed) {
    for (int i = 3; i < argc; ++i) {
        const std::string arg = argv[i];
        std::vector<double> values;
        std::string error;
        if (arg == "--get-state") {
            parsed.get_motion_state = true;
        } else if (arg == "--monitor-state") {
            parsed.monitor_motion_state = true;
        } else if (arg == "--position") {
            if (!parseDoubleArray(argc, argv, i, 3, values, error)) {
                std::cerr << "--position: " << error << ".\n";
                return false;
            }
            parsed.command.position = {values[0], values[1], values[2]};
            parsed.has_setter = true;
        } else if (arg == "--lin-velocity") {
            if (!parseDoubleArray(argc, argv, i, 3, values, error)) {
                std::cerr << "--lin-velocity: " << error << ".\n";
                return false;
            }
            parsed.command.lin_velocity = {values[0], values[1], values[2]};
            parsed.has_setter = true;
        } else if (arg == "--attitude") {
            if (!parseDoubleArray(argc, argv, i, 3, values, error)) {
                std::cerr << "--attitude: " << error << ".\n";
                return false;
            }
            parsed.command.attitude = {values[0], values[1], values[2]};
            parsed.has_setter = true;
        } else if (arg == "--ang-velocity") {
            if (!parseDoubleArray(argc, argv, i, 3, values, error)) {
                std::cerr << "--ang-velocity: " << error << ".\n";
                return false;
            }
            parsed.command.ang_velocity = {values[0], values[1], values[2]};
            parsed.has_setter = true;
        } else if (arg == "--delta-height") {
            if (i + 1 >= argc || !parseDouble(argv[++i], parsed.command.delta_height)) {
                std::cerr << "--delta-height requires one numeric value.\n";
                return false;
            }
            parsed.has_setter = true;
        } else if (arg == "--gait-type") {
            if (i + 1 >= argc || !parseUint8(argv[++i], parsed.command.gait_type)) {
                std::cerr << "--gait-type requires one uint8 value.\n";
                return false;
            }
            parsed.has_setter = true;
        } else if (arg == "--skills") {
            if (!parseUint8Array(argc, argv, i, 8, parsed.command.skills, error)) {
                std::cerr << "--skills: " << error << ".\n";
                return false;
            }
            parsed.has_setter = true;
        } else if (arg == "--duration") {
            if (i + 1 >= argc || !parseDouble(argv[++i], parsed.command.duration)) {
                std::cerr << "--duration requires one numeric value.\n";
                return false;
            }
            parsed.has_setter = true;
        } else {
            std::cerr << "Unknown argument: " << arg << "\n";
            return false;
        }
    }
    return true;
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
    if (!parseArgs(argc, argv, parsed)) {
        printUsage(argv[0]);
        return 1;
    }
    if (parsed.monitor_motion_state &&
        (parsed.get_motion_state || parsed.has_setter)) {
        std::cerr << "--monitor-state cannot be combined with other operations.\n";
        return 1;
    }

    fourier::aurora_sdk::AuroraClient* client = nullptr;
    try {
        client = &fourier::aurora_sdk::AuroraClient::getInstance(config.user_id, config.domain_id);
        std::this_thread::sleep_for(std::chrono::seconds(1));

        if (parsed.monitor_motion_state) {
            while (!example::stopRequested()) {
                printMotionState(waitForMotionState(*client));
                std::cout << "------------------------------\n";
                std::this_thread::sleep_for(std::chrono::seconds(1));
            }
        }

        if (parsed.get_motion_state) {
            printMotionState(waitForMotionState(*client));
        }

        if (parsed.has_setter) {
            client->publishMotionCommand(parsed.command);
            std::cout << "publishMotionCommand success\n";
            example::closeClientOrForceExit(client, 0);
            return 0;
        }

        example::closeClientOrForceExit(client, 0);
    } catch (const std::exception& error) {
        std::cerr << "motion_client failed: " << error.what() << "\n";
        example::closeClientOrForceExit(client, 1);
        return 1;
    }
}
