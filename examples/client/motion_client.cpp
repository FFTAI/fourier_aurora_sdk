#include <array>
#include <chrono>
#include <cstdint>
#include <exception>
#include <iostream>
#include <string>
#include <vector>

#include "client_common.hpp"

namespace sdk = fourier::aurora_sdk;

namespace {

struct Arguments {
    bool get_state = false;
    bool monitor_state = false;
    bool get_estimator = false;
    bool monitor_estimator = false;
    bool publish_command = false;
    sdk::MotionCommand command;
};

void printUsage(const char* program) {
    std::cerr
        << "Usage: " << program << " <domain_id> <source_id> <operation...>\n"
        << "Read operations:\n"
        << "  --get-state\n"
        << "  --monitor-state\n"
        << "  --get-estimator\n"
        << "  --monitor-estimator\n"
        << "Command fields (one command is published):\n"
        << "  --position <x> <y> <z>\n"
        << "  --lin-velocity <vx> <vy> <vz>\n"
        << "  --attitude <roll> <pitch> <yaw>\n"
        << "  --ang-velocity <wx> <wy> <wz>\n"
        << "  --delta-height <value>\n"
        << "  --gait-type <uint8>\n"
        << "  --skills <s0> <s1> <s2> <s3> <s4> <s5> <s6> <s7>\n"
        << "  --duration <seconds>\n";
}

bool parseDoubleArray(
    int argc,
    char** argv,
    int& index,
    std::size_t count,
    std::vector<double>& values) {
    if (index + static_cast<int>(count) >= argc) {
        return false;
    }
    values.clear();
    values.reserve(count);
    for (std::size_t offset = 0; offset < count; ++offset) {
        double value = 0.0;
        if (!client_example::parseDouble(argv[++index], value)) {
            return false;
        }
        values.push_back(value);
    }
    return true;
}

bool parseArguments(
    int argc,
    char** argv,
    Arguments& parsed,
    std::string& error) {
    for (int index = 3; index < argc; ++index) {
        const std::string argument = argv[index];
        std::vector<double> values;
        if (argument == "--get-state") {
            parsed.get_state = true;
        } else if (argument == "--monitor-state") {
            parsed.monitor_state = true;
        } else if (argument == "--get-estimator") {
            parsed.get_estimator = true;
        } else if (argument == "--monitor-estimator") {
            parsed.monitor_estimator = true;
        } else if (argument == "--position") {
            if (!parseDoubleArray(argc, argv, index, 3, values)) {
                error = "--position requires three finite values";
                return false;
            }
            parsed.command.position = {values[0], values[1], values[2]};
            parsed.publish_command = true;
        } else if (argument == "--lin-velocity") {
            if (!parseDoubleArray(argc, argv, index, 3, values)) {
                error = "--lin-velocity requires three finite values";
                return false;
            }
            parsed.command.lin_velocity = {values[0], values[1], values[2]};
            parsed.publish_command = true;
        } else if (argument == "--attitude") {
            if (!parseDoubleArray(argc, argv, index, 3, values)) {
                error = "--attitude requires three finite values";
                return false;
            }
            parsed.command.attitude = {values[0], values[1], values[2]};
            parsed.publish_command = true;
        } else if (argument == "--ang-velocity") {
            if (!parseDoubleArray(argc, argv, index, 3, values)) {
                error = "--ang-velocity requires three finite values";
                return false;
            }
            parsed.command.ang_velocity = {values[0], values[1], values[2]};
            parsed.publish_command = true;
        } else if (argument == "--delta-height") {
            if (++index >= argc ||
                !client_example::parseDouble(
                    argv[index], parsed.command.delta_height)) {
                error = "--delta-height requires one finite value";
                return false;
            }
            parsed.publish_command = true;
        } else if (argument == "--gait-type") {
            if (++index >= argc ||
                !client_example::parseUint8(
                    argv[index], parsed.command.gait_type)) {
                error = "--gait-type requires one uint8 value";
                return false;
            }
            parsed.publish_command = true;
        } else if (argument == "--skills") {
            if (index + 8 >= argc) {
                error = "--skills requires eight uint8 values";
                return false;
            }
            for (std::size_t skill = 0;
                 skill < parsed.command.skills.size();
                 ++skill) {
                if (!client_example::parseUint8(
                        argv[++index], parsed.command.skills[skill])) {
                    error = "--skills requires eight uint8 values";
                    return false;
                }
            }
            parsed.publish_command = true;
        } else if (argument == "--duration") {
            if (++index >= argc ||
                !client_example::parseDouble(
                    argv[index], parsed.command.duration) ||
                parsed.command.duration < 0.0) {
                error = "--duration requires one nonnegative finite value";
                return false;
            }
            parsed.publish_command = true;
        } else {
            error = "unknown operation: " + argument;
            return false;
        }
    }

    const int monitor_count =
        static_cast<int>(parsed.monitor_state) +
        static_cast<int>(parsed.monitor_estimator);
    const bool has_other =
        parsed.get_state || parsed.get_estimator || parsed.publish_command;
    if (monitor_count > 1 || (monitor_count != 0 && has_other)) {
        error = "a monitor option must be the only operation";
        return false;
    }
    if (monitor_count == 0 && !has_other) {
        error = "at least one operation is required";
        return false;
    }
    return true;
}

void printMotionState(const sdk::MotionState& state) {
    std::cout << "------------------------------\n";
    client_example::printHeader(state.header);
    std::cout << "position: ";
    client_example::printRange(state.position);
    std::cout << "\nposition_cmd: ";
    client_example::printRange(state.position_cmd);
    std::cout << "\nlin_velocity: ";
    client_example::printRange(state.lin_velocity);
    std::cout << "\nlin_velocity_cmd: ";
    client_example::printRange(state.lin_velocity_cmd);
    std::cout << "\nattitude: ";
    client_example::printRange(state.attitude);
    std::cout << "\nattitude_cmd: ";
    client_example::printRange(state.attitude_cmd);
    std::cout << "\nang_velocity: ";
    client_example::printRange(state.ang_velocity);
    std::cout << "\nang_velocity_cmd: ";
    client_example::printRange(state.ang_velocity_cmd);
    std::cout << "\ndelta_height: " << state.delta_height
              << "\ndelta_height_cmd: " << state.delta_height_cmd
              << "\nprogress: " << static_cast<int>(state.progress)
              << "\nstable_level: " << static_cast<int>(state.stable_level)
              << "\n";
}

void printEstimator(const sdk::EstimatorData& data) {
    std::cout << "------------------------------\n";
    client_example::printHeader(data.header);
    std::cout << "quat_xyzw: ";
    client_example::printRange(data.quat_xyzw);
    std::cout << "\nquat_wxyz: ";
    client_example::printRange(data.quat_wxyz);
    std::cout << "\nrpy: ";
    client_example::printRange(data.rpy);
    std::cout << "\nomega_W: ";
    client_example::printRange(data.omega_W);
    std::cout << "\nacc_W: ";
    client_example::printRange(data.acc_W);
    std::cout << "\nomega_B: ";
    client_example::printRange(data.omega_B);
    std::cout << "\nacc_B: ";
    client_example::printRange(data.acc_B);
    std::cout << "\nvel_W: ";
    client_example::printRange(data.vel_W);
    std::cout << "\npos_W: ";
    client_example::printRange(data.pos_W);
    std::cout << "\nvel_B: ";
    client_example::printRange(data.vel_B);
    std::cout << "\ncontact_force: ";
    client_example::printRange(data.contact_force);
    std::cout << "\ncontact_prob: ";
    client_example::printRange(data.contact_prob);
    std::cout << "\nstable_level: "
              << static_cast<int>(data.stable_level) << "\n";
}

int monitorMotionState(sdk::AuroraClient& client) {
    client_example::FailureSnapshot failure;
    while (!client_example::stopRequested()) {
        sdk::MotionState state;
        const auto result = client.getMotionState(state);
        if (result.success()) {
            failure.clear();
            printMotionState(state);
        } else if (failure.changed(result)) {
            client_example::printResult("getMotionState", result);
        }
        client_example::sleepInterruptibly(std::chrono::milliseconds(500));
    }
    return client_example::kInterrupted;
}

int monitorEstimator(sdk::AuroraClient& client) {
    client_example::FailureSnapshot failure;
    while (!client_example::stopRequested()) {
        sdk::EstimatorData data;
        const auto result = client.getEstimatorData(data);
        if (result.success()) {
            failure.clear();
            printEstimator(data);
        } else if (failure.changed(result)) {
            client_example::printResult("getEstimatorData", result);
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
        client_example::configureClient(
            client,
            domain_id,
            argv[2],
            {sdk::Endpoint::ManageLeaseService,
             sdk::Endpoint::MotionStateSubscriber,
             sdk::Endpoint::EstimatorDataSubscriber,
             sdk::Endpoint::MotionCommandPublisher});
        client.start();
        const auto lease = client.registerLease({sdk::AuroraResource::MotionCommand});
        if (!lease.success()) {
            client_example::printResult("registerLease", lease.operation);
            return client_example::stopClient(client, client_example::kRuntimeFailure);
        }
        client_example::sleepInterruptibly(std::chrono::milliseconds(500));

        int exit_code = client_example::kSuccess;
        if (parsed.monitor_state) {
            exit_code = monitorMotionState(client);
        } else if (parsed.monitor_estimator) {
            exit_code = monitorEstimator(client);
        } else if (client_example::stopRequested()) {
            exit_code = client_example::kInterrupted;
        } else {
            if (parsed.get_state) {
                sdk::MotionState state;
                const auto result = client.getMotionState(state);
                if (!result.success()) {
                    client_example::printResult("getMotionState", result);
                    exit_code = client_example::kRuntimeFailure;
                } else {
                    printMotionState(state);
                }
            }
            if (exit_code == client_example::kSuccess &&
                parsed.get_estimator) {
                sdk::EstimatorData data;
                const auto result = client.getEstimatorData(data);
                if (!result.success()) {
                    client_example::printResult("getEstimatorData", result);
                    exit_code = client_example::kRuntimeFailure;
                } else {
                    printEstimator(data);
                }
            }
            if (exit_code == client_example::kSuccess &&
                parsed.publish_command) {
                const auto result =
                    client.publishMotionCommand(parsed.command);
                client_example::printResult(
                    "publishMotionCommand", result, std::cout);
                if (!result.success()) {
                    exit_code = client_example::kRuntimeFailure;
                }
            }
        }
        return client_example::stopClient(client, exit_code);
    } catch (const std::exception& error) {
        std::cerr << "motion_client failed: " << error.what() << "\n";
        return client_example::stopClient(
            client, client_example::kRuntimeFailure);
    }
}
