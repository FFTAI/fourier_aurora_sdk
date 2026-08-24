#include <algorithm>
#include <array>
#include <chrono>
#include <cstdint>
#include <exception>
#include <iomanip>
#include <iostream>
#include <limits>
#include <optional>
#include <random>
#include <sstream>
#include <string>
#include <string_view>
#include <thread>
#include <type_traits>
#include <utility>
#include <variant>
#include <vector>

#include "client_common.hpp"

namespace sdk = fourier::aurora_sdk;

namespace {

constexpr uint32_t kDefaultDomainId = 123;
constexpr auto kDefaultMonitorInterval = std::chrono::milliseconds(500);
constexpr auto kEndpointTimeout = std::chrono::seconds(5);

struct GlobalOptions {
    uint32_t domain_id = kDefaultDomainId;
    std::optional<std::string> source_name;
};

enum class ReadMode { Get, Monitor };

struct StateCommand {
    enum class Action { Get, List, Request, Monitor } action;
    uint32_t state_id = 0;
    std::chrono::milliseconds interval = kDefaultMonitorInterval;
};

struct EstimatorCommand {
    ReadMode mode;
    std::chrono::milliseconds interval = kDefaultMonitorInterval;
};

struct MotionCommand {
    enum class Action { Get, Monitor, Publish } action;
    std::chrono::milliseconds interval = kDefaultMonitorInterval;
    sdk::MotionCommand command;
};

struct PositionTarget {
    std::string group_name;
    std::vector<double> position;
};

struct ControlGroupCommand {
    enum class Action { Get, Monitor, Position } action;
    std::chrono::milliseconds interval = kDefaultMonitorInterval;
    std::vector<PositionTarget> targets;
    double duration = 5.0;
    double rate_hz = 100.0;
};

using Command = std::variant<
    StateCommand,
    EstimatorCommand,
    MotionCommand,
    ControlGroupCommand>;

void printRootHelp(std::ostream& output) {
    output
        << "Usage: aurora-client [global options] <area> <command> [options]\n"
        << "\nAreas:\n"
        << "  state          Aurora and FSM state\n"
        << "  estimator      Read-only estimator telemetry\n"
        << "  motion         Motion state and command publishing\n"
        << "  control-group  Joint and Cartesian control-group data\n"
        << "\nGlobal options (accepted anywhere):\n"
        << "  -d, --domain_id <uint32>  DDS domain ID (default: 123)\n"
        << "  -s, --source_name <name>   Source Name (default: aurora-client-NNNN)\n"
        << "  -h, --help                Show help\n"
        << "      --version             Show version\n"
        << "\nRun 'aurora-client <area> --help' for area commands.\n";
}

void printStateHelp(std::ostream& output) {
    output
        << "Usage: aurora-client [global options] state <command> [options]\n"
        << "\nCommands:\n"
        << "  get                         Print current Aurora state\n"
        << "  list                        Print advertised FSM states\n"
        << "  set <state-id>              Request an FSM transition\n"
        << "  monitor [--interval <time>] Monitor state (default: 500ms)\n";
}

void printEstimatorHelp(std::ostream& output) {
    output
        << "Usage: aurora-client [global options] estimator <command> [options]\n"
        << "\nCommands:\n"
        << "  get                         Print estimator telemetry\n"
        << "  monitor [--interval <time>] Monitor telemetry (default: 500ms)\n";
}

void printMotionHelp(std::ostream& output) {
    output
        << "Usage: aurora-client [global options] motion <command> [options]\n"
        << "\nCommands:\n"
        << "  get                         Print motion state\n"
        << "  monitor [--interval <time>] Monitor motion state (default: 500ms)\n"
        << "  set <fields...>             Publish one motion command\n"
        << "\nSet fields (at least one required):\n"
        << "  --position <x> <y> <z>\n"
        << "  --linear-velocity <vx> <vy> <vz>\n"
        << "  --attitude <roll> <pitch> <yaw>\n"
        << "  --angular-velocity <wx> <wy> <wz>\n"
        << "  --delta-height <value>\n"
        << "  --gait-type <uint8>\n"
        << "  --skills <s0> <s1> <s2> <s3> <s4> <s5> <s6> <s7>\n"
        << "  --duration <seconds>\n";
}

void printControlGroupHelp(std::ostream& output) {
    output
        << "Usage: aurora-client [global options] control-group <command> [options]\n"
        << "\nCommands:\n"
        << "  get                         Print control-group state\n"
        << "  monitor [--interval <time>] Monitor state (default: 500ms)\n"
        << "  set --target <group> <joint...> [options]\n"
        << "\nSet options:\n"
        << "  --target <group> <joint...> Repeatable joint target\n"
        << "  --duration <seconds>        Interpolation duration (default: 5s)\n"
        << "  --rate <hz>                 Publication rate (default: 100Hz)\n";
}

bool printCommandHelp(
    const std::string& area,
    const std::string& command,
    std::ostream& output) {
    if (area == "state") {
        if (command == "get" || command == "list") {
            output << "Usage: aurora-client [global options] state "
                   << command << "\n";
            return true;
        }
        if (command == "set") {
            output << "Usage: aurora-client [global options] state set <state-id>\n";
            return true;
        }
        if (command == "monitor") {
            output << "Usage: aurora-client [global options] state monitor "
                      "[--interval <time>]\n"
                   << "  --interval <time>  Polling interval (default: 500ms; minimum: 10ms)\n";
            return true;
        }
    } else if (area == "estimator") {
        if (command == "get") {
            output << "Usage: aurora-client [global options] estimator get\n";
            return true;
        }
        if (command == "monitor") {
            output << "Usage: aurora-client [global options] estimator monitor "
                      "[--interval <time>]\n"
                   << "  --interval <time>  Polling interval (default: 500ms; minimum: 10ms)\n";
            return true;
        }
    } else if (area == "motion") {
        if (command == "get") {
            output << "Usage: aurora-client [global options] motion get\n";
            return true;
        }
        if (command == "monitor") {
            output << "Usage: aurora-client [global options] motion monitor "
                      "[--interval <time>]\n"
                   << "  --interval <time>  Polling interval (default: 500ms; minimum: 10ms)\n";
            return true;
        }
        if (command == "set") {
            printMotionHelp(output);
            return true;
        }
    } else if (area == "control-group") {
        if (command == "get") {
            output << "Usage: aurora-client [global options] control-group get\n";
            return true;
        }
        if (command == "monitor") {
            output << "Usage: aurora-client [global options] control-group monitor "
                      "[--interval <time>]\n"
                   << "  --interval <time>  Polling interval (default: 500ms; minimum: 10ms)\n";
            return true;
        }
        if (command == "set") {
            printControlGroupHelp(output);
            return true;
        }
    }
    return false;
}

bool isHelp(const std::string& value) {
    return value == "-h" || value == "--help";
}

bool containsHelp(const std::vector<std::string>& args) {
    return std::any_of(args.begin(), args.end(), isHelp);
}

bool validSourceName(std::string_view name) {
    return !name.empty() && name.size() <= 64 &&
           std::all_of(name.begin(), name.end(), [](unsigned char c) {
               return (c >= 'A' && c <= 'Z') || (c >= 'a' && c <= 'z') ||
                      (c >= '0' && c <= '9') || c == '.' || c == '_' || c == '-';
           });
}

std::string generateDefaultSourceName() {
    std::random_device random;
    std::uniform_int_distribution<unsigned int> suffix(0, 9999);
    std::ostringstream out;
    out << "aurora-client-" << std::setfill('0') << std::setw(4) << suffix(random);
    return out.str();
}

bool parseGlobalOptions(
    int argc,
    char** argv,
    GlobalOptions& options,
    std::vector<std::string>& remaining,
    std::string& error) {
    bool domain_seen = false;
    bool source_seen = false;
    for (int index = 1; index < argc; ++index) {
        const std::string argument = argv[index];
        const bool is_domain = argument == "-d" || argument == "--domain_id";
        const bool is_source = argument == "-s" || argument == "--source_name";
        if (!is_domain && !is_source) {
            remaining.push_back(argument);
            continue;
        }
        bool& seen = is_domain ? domain_seen : source_seen;
        if (seen) {
            error = "option specified more than once: " + argument;
            return false;
        }
        seen = true;
        if (++index >= argc) {
            error = argument + (is_source ? " requires a Source Name" : " requires a uint32 value");
            return false;
        }
        if (is_domain) {
            uint32_t value = 0;
            if (!client_example::parseUint32(argv[index], value)) {
                error = argument + " requires a uint32 value";
                return false;
            }
            options.domain_id = value;
        } else {
            const std::string value = argv[index];
            if (!validSourceName(value)) {
                error = argument + " requires [A-Za-z0-9._-]{1,64}";
                return false;
            }
            options.source_name = value;
        }
    }
    return true;
}

bool parseInterval(
    const std::string& text,
    std::chrono::milliseconds& interval,
    std::string& error) {
    std::string number = text;
    double multiplier = 1.0;
    if (number.size() >= 2 &&
        number.compare(number.size() - 2, 2, "ms") == 0) {
        number.resize(number.size() - 2);
    } else if (!number.empty() && number.back() == 's') {
        number.pop_back();
        multiplier = 1000.0;
    }
    double value = 0.0;
    if (!client_example::parseDouble(number, value) || value <= 0.0) {
        error = "--interval requires a positive duration in ms or s";
        return false;
    }
    const double milliseconds = value * multiplier;
    if (!std::isfinite(milliseconds) || milliseconds < 10.0 ||
        milliseconds > static_cast<double>(
            std::numeric_limits<int64_t>::max())) {
        error = "--interval must be at least 10ms and representable";
        return false;
    }
    interval = std::chrono::milliseconds(
        static_cast<int64_t>(milliseconds));
    return true;
}

bool parseMonitorOptions(
    const std::vector<std::string>& args,
    std::chrono::milliseconds& interval,
    std::string& error) {
    bool interval_seen = false;
    for (std::size_t index = 0; index < args.size(); ++index) {
        if (args[index] != "--interval") {
            error = "unknown monitor option: " + args[index];
            return false;
        }
        if (interval_seen) {
            error = "option specified more than once: --interval";
            return false;
        }
        interval_seen = true;
        if (++index >= args.size() ||
            !parseInterval(args[index], interval, error)) {
            if (error.empty()) {
                error = "--interval requires a duration";
            }
            return false;
        }
    }
    return true;
}

bool parseDoubleArray(
    const std::vector<std::string>& args,
    std::size_t& index,
    std::size_t count,
    std::vector<double>& values) {
    if (index + count >= args.size()) {
        return false;
    }
    values.clear();
    values.reserve(count);
    for (std::size_t offset = 0; offset < count; ++offset) {
        double value = 0.0;
        if (!client_example::parseDouble(args[++index], value)) {
            return false;
        }
        values.push_back(value);
    }
    return true;
}

bool parseState(
    const std::vector<std::string>& args,
    StateCommand& parsed,
    std::string& error) {
    const std::string& action = args.front();
    std::vector<std::string> options(args.begin() + 1, args.end());
    if (action == "get" || action == "list") {
        if (!options.empty()) {
            error = action + " does not accept command options";
            return false;
        }
        parsed.action = action == "get"
            ? StateCommand::Action::Get
            : StateCommand::Action::List;
        return true;
    }
    if (action == "set") {
        if (options.size() != 1 ||
            !client_example::parseUint32(options.front(), parsed.state_id)) {
            error = "state set requires one uint32 state ID";
            return false;
        }
        parsed.action = StateCommand::Action::Request;
        return true;
    }
    if (action == "monitor") {
        parsed.action = StateCommand::Action::Monitor;
        return parseMonitorOptions(options, parsed.interval, error);
    }
    error = "unknown state command: " + action;
    return false;
}

bool parseEstimator(
    const std::vector<std::string>& args,
    EstimatorCommand& parsed,
    std::string& error) {
    const std::string& action = args.front();
    std::vector<std::string> options(args.begin() + 1, args.end());
    if (action == "get") {
        if (!options.empty()) {
            error = "estimator get does not accept command options";
            return false;
        }
        parsed.mode = ReadMode::Get;
        return true;
    }
    if (action == "monitor") {
        parsed.mode = ReadMode::Monitor;
        return parseMonitorOptions(options, parsed.interval, error);
    }
    error = "unknown estimator command: " + action;
    return false;
}

bool parseMotionPublish(
    const std::vector<std::string>& args,
    MotionCommand& parsed,
    std::string& error) {
    bool any = false;
    bool position = false;
    bool linear = false;
    bool attitude = false;
    bool angular = false;
    bool height = false;
    bool gait = false;
    bool skills = false;
    bool duration = false;
    for (std::size_t index = 0; index < args.size(); ++index) {
        const std::string& argument = args[index];
        std::vector<double> values;
        bool* seen = nullptr;
        if (argument == "--position") seen = &position;
        else if (argument == "--linear-velocity") seen = &linear;
        else if (argument == "--attitude") seen = &attitude;
        else if (argument == "--angular-velocity") seen = &angular;
        else if (argument == "--delta-height") seen = &height;
        else if (argument == "--gait-type") seen = &gait;
        else if (argument == "--skills") seen = &skills;
        else if (argument == "--duration") seen = &duration;
        else {
            error = "unknown motion set option: " + argument;
            return false;
        }
        if (*seen) {
            error = "option specified more than once: " + argument;
            return false;
        }
        *seen = true;
        any = true;
        if (argument == "--position" || argument == "--linear-velocity" ||
            argument == "--attitude" || argument == "--angular-velocity") {
            if (!parseDoubleArray(args, index, 3, values)) {
                error = argument + " requires three finite values";
                return false;
            }
            const std::array<double, 3> value{values[0], values[1], values[2]};
            if (argument == "--position") parsed.command.position = value;
            else if (argument == "--linear-velocity") parsed.command.lin_velocity = value;
            else if (argument == "--attitude") parsed.command.attitude = value;
            else parsed.command.ang_velocity = value;
        } else if (argument == "--delta-height") {
            if (++index >= args.size() ||
                !client_example::parseDouble(args[index], parsed.command.delta_height)) {
                error = argument + " requires one finite value";
                return false;
            }
        } else if (argument == "--gait-type") {
            if (++index >= args.size() ||
                !client_example::parseUint8(args[index], parsed.command.gait_type)) {
                error = argument + " requires one uint8 value";
                return false;
            }
        } else if (argument == "--skills") {
            if (index + parsed.command.skills.size() >= args.size()) {
                error = argument + " requires eight uint8 values";
                return false;
            }
            for (auto& skill : parsed.command.skills) {
                if (!client_example::parseUint8(args[++index], skill)) {
                    error = argument + " requires eight uint8 values";
                    return false;
                }
            }
        } else {
            if (++index >= args.size() ||
                !client_example::parseDouble(args[index], parsed.command.duration) ||
                parsed.command.duration < 0.0) {
                error = argument + " requires one nonnegative finite value";
                return false;
            }
        }
    }
    if (!any) {
        error = "motion set requires at least one command field";
        return false;
    }
    return true;
}

bool parseMotion(
    const std::vector<std::string>& args,
    MotionCommand& parsed,
    std::string& error) {
    const std::string& action = args.front();
    std::vector<std::string> options(args.begin() + 1, args.end());
    if (action == "get") {
        if (!options.empty()) {
            error = "motion get does not accept command options";
            return false;
        }
        parsed.action = MotionCommand::Action::Get;
        return true;
    }
    if (action == "monitor") {
        parsed.action = MotionCommand::Action::Monitor;
        return parseMonitorOptions(options, parsed.interval, error);
    }
    if (action == "set") {
        parsed.action = MotionCommand::Action::Publish;
        return parseMotionPublish(options, parsed, error);
    }
    error = "unknown motion command: " + action;
    return false;
}

bool parseControlGroupSet(
    const std::vector<std::string>& args,
    ControlGroupCommand& parsed,
    std::string& error) {
    bool duration_seen = false;
    bool rate_seen = false;
    for (std::size_t index = 0; index < args.size(); ++index) {
        const std::string& argument = args[index];
        if (argument == "--target") {
            if (++index >= args.size()) {
                error = "--target requires a group name and joint values";
                return false;
            }
            PositionTarget target;
            target.group_name = args[index];
            while (index + 1 < args.size() &&
                   args[index + 1].rfind("--", 0) != 0) {
                double value = 0.0;
                if (!client_example::parseDouble(args[++index], value)) {
                    error = "--target joint values must be finite numbers";
                    return false;
                }
                target.position.push_back(value);
            }
            if (target.group_name.empty() || target.position.empty()) {
                error = "--target requires a group name and joint values";
                return false;
            }
            parsed.targets.push_back(std::move(target));
        } else if (argument == "--duration" || argument == "--rate") {
            bool& seen = argument == "--duration" ? duration_seen : rate_seen;
            if (seen) {
                error = "option specified more than once: " + argument;
                return false;
            }
            seen = true;
            double& value = argument == "--duration"
                ? parsed.duration
                : parsed.rate_hz;
            if (++index >= args.size() ||
                !client_example::parseDouble(args[index], value) || value <= 0.0) {
                error = argument + " requires a positive finite value";
                return false;
            }
        } else {
            error = "unknown control-group set option: " + argument;
            return false;
        }
    }
    if (parsed.targets.empty()) {
        error = "control-group set requires at least one --target";
        return false;
    }
    return true;
}

bool parseControlGroup(
    const std::vector<std::string>& args,
    ControlGroupCommand& parsed,
    std::string& error) {
    const std::string& action = args.front();
    std::vector<std::string> options(args.begin() + 1, args.end());
    if (action == "get") {
        if (!options.empty()) {
            error = "control-group get does not accept command options";
            return false;
        }
        parsed.action = ControlGroupCommand::Action::Get;
        return true;
    }
    if (action == "monitor") {
        parsed.action = ControlGroupCommand::Action::Monitor;
        return parseMonitorOptions(options, parsed.interval, error);
    }
    if (action == "set") {
        parsed.action = ControlGroupCommand::Action::Position;
        return parseControlGroupSet(options, parsed, error);
    }
    error = "unknown control-group command: " + action;
    return false;
}

std::vector<sdk::Endpoint> endpointsFor(const Command& command) {
    return std::visit([](const auto& selected) {
        using T = std::decay_t<decltype(selected)>;
        if constexpr (std::is_same_v<T, StateCommand>) {
            if (selected.action == StateCommand::Action::Request) {
                return std::vector<sdk::Endpoint>{sdk::Endpoint::FsmService};
            }
            return std::vector<sdk::Endpoint>{sdk::Endpoint::AuroraStateSubscriber};
        } else if constexpr (std::is_same_v<T, EstimatorCommand>) {
            return std::vector<sdk::Endpoint>{sdk::Endpoint::EstimatorDataSubscriber};
        } else if constexpr (std::is_same_v<T, MotionCommand>) {
            if (selected.action == MotionCommand::Action::Publish) {
                return std::vector<sdk::Endpoint>{
                    sdk::Endpoint::ManageLeaseService,
                    sdk::Endpoint::MotionCommandPublisher};
            }
            return std::vector<sdk::Endpoint>{sdk::Endpoint::MotionStateSubscriber};
        } else {
            if (selected.action == ControlGroupCommand::Action::Position) {
                return std::vector<sdk::Endpoint>{
                    sdk::Endpoint::ControlGroupStateSubscriber,
                    sdk::Endpoint::ManageLeaseService,
                    sdk::Endpoint::ControlGroupCommandPublisher};
            }
            return std::vector<sdk::Endpoint>{sdk::Endpoint::ControlGroupStateSubscriber};
        }
    }, command);
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
              << "end_effector_type: " << state.robot_info.end_effector_type << "\n"
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

void printMotionState(const sdk::MotionState& state) {
    std::cout << "------------------------------\n";
    client_example::printHeader(state.header);
    std::cout << "position: "; client_example::printRange(state.position);
    std::cout << "\nposition_cmd: "; client_example::printRange(state.position_cmd);
    std::cout << "\nlin_velocity: "; client_example::printRange(state.lin_velocity);
    std::cout << "\nlin_velocity_cmd: "; client_example::printRange(state.lin_velocity_cmd);
    std::cout << "\nattitude: "; client_example::printRange(state.attitude);
    std::cout << "\nattitude_cmd: "; client_example::printRange(state.attitude_cmd);
    std::cout << "\nang_velocity: "; client_example::printRange(state.ang_velocity);
    std::cout << "\nang_velocity_cmd: "; client_example::printRange(state.ang_velocity_cmd);
    std::cout << "\ndelta_height: " << state.delta_height
              << "\ndelta_height_cmd: " << state.delta_height_cmd
              << "\nprogress: " << static_cast<int>(state.progress)
              << "\nstable_level: " << static_cast<int>(state.stable_level) << "\n";
}

void printEstimator(const sdk::EstimatorData& data) {
    std::cout << "------------------------------\n";
    client_example::printHeader(data.header);
    std::cout << "quat_xyzw: "; client_example::printRange(data.quat_xyzw);
    std::cout << "\nquat_wxyz: "; client_example::printRange(data.quat_wxyz);
    std::cout << "\nrpy: "; client_example::printRange(data.rpy);
    std::cout << "\nomega_W: "; client_example::printRange(data.omega_W);
    std::cout << "\nacc_W: "; client_example::printRange(data.acc_W);
    std::cout << "\nomega_B: "; client_example::printRange(data.omega_B);
    std::cout << "\nacc_B: "; client_example::printRange(data.acc_B);
    std::cout << "\nvel_W: "; client_example::printRange(data.vel_W);
    std::cout << "\npos_W: "; client_example::printRange(data.pos_W);
    std::cout << "\nvel_B: "; client_example::printRange(data.vel_B);
    std::cout << "\ncontact_force: "; client_example::printRange(data.contact_force);
    std::cout << "\ncontact_prob: "; client_example::printRange(data.contact_prob);
    std::cout << "\nstable_level: " << static_cast<int>(data.stable_level) << "\n";
}

const sdk::ControlGroupStateEntry* findGroup(
    const std::vector<sdk::ControlGroupStateEntry>& groups,
    const std::string& name) {
    const auto found = std::find_if(groups.begin(), groups.end(),
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
    std::cout << "\nvelocity: "; client_example::printRange(state.joint_velocity);
    std::cout << "\neffort: "; client_example::printRange(state.joint_effort);
    std::cout << "\nposition_cmd: ";
    if (command) client_example::printRange(command->joint_position);
    else client_example::printRange(std::vector<double>{});
    std::cout << "\nvelocity_cmd: ";
    if (command) client_example::printRange(command->joint_velocity);
    else client_example::printRange(std::vector<double>{});
    std::cout << "\neffort_cmd: ";
    if (command) client_example::printRange(command->joint_effort);
    else client_example::printRange(std::vector<double>{});
    std::cout << "\n";
}

void printCartesian(const sdk::CartesianStateEntry& state) {
    std::cout << "link_name: " << state.link_name << "\npose: ";
    client_example::printRange(state.pose);
    std::cout << "\ntwist: "; client_example::printRange(state.twist);
    std::cout << "\nwrench: "; client_example::printRange(state.wrench);
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

template <typename Sample, typename Getter, typename Printer>
int monitor(
    sdk::AuroraClient& client,
    std::chrono::milliseconds interval,
    const char* operation,
    Getter getter,
    Printer printer) {
    client_example::FailureSnapshot failure;
    while (!client_example::stopRequested()) {
        Sample sample;
        const auto result = getter(client, sample);
        if (result.success()) {
            failure.clear();
            printer(sample);
        } else if (failure.changed(result)) {
            client_example::printResult(operation, result);
        }
        client_example::sleepInterruptibly(interval);
    }
    return client_example::kInterrupted;
}

sdk::AuroraResource resourceForGroup(const std::string& group) {
    if (group == "left_leg") return sdk::AuroraResource::LeftLeg;
    if (group == "right_leg") return sdk::AuroraResource::RightLeg;
    if (group == "waist") return sdk::AuroraResource::Waist;
    if (group == "head") return sdk::AuroraResource::Head;
    if (group == "left_manipulator") return sdk::AuroraResource::LeftManipulator;
    if (group == "right_manipulator") return sdk::AuroraResource::RightManipulator;
    if (group == "left_end_effector") return sdk::AuroraResource::LeftEndEffector;
    if (group == "right_end_effector") return sdk::AuroraResource::RightEndEffector;
    throw sdk::InvalidArgumentError("unsupported control group: " + group);
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

int setPositions(sdk::AuroraClient& client, const ControlGroupCommand& parsed) {
    sdk::ControlGroupState state;
    const auto state_result = client.getControlGroupState(state);
    if (!state_result.success()) {
        client_example::printResult("getControlGroupState", state_result);
        return client_example::kRuntimeFailure;
    }
    std::vector<std::vector<double>> starts;
    std::vector<sdk::AuroraResource> resources;
    for (const auto& target : parsed.targets) {
        const auto* group = findGroup(state.joint_state, target.group_name);
        if (!group) {
            std::cerr << "group not found: " << target.group_name << "\n";
            return client_example::kRuntimeFailure;
        }
        if (group->joint_position.size() != target.position.size()) {
            std::cerr << "target length for " << target.group_name << " is "
                      << target.position.size() << ", expected "
                      << group->joint_position.size() << "\n";
            return client_example::kRuntimeFailure;
        }
        starts.push_back(group->joint_position);
        const auto resource = resourceForGroup(target.group_name);
        if (std::find(resources.begin(), resources.end(), resource) == resources.end()) {
            resources.push_back(resource);
        }
    }
    const auto lease = client.registerLease(resources);
    if (!lease.success()) {
        client_example::printResult("registerLease", lease.operation);
        return client_example::kRuntimeFailure;
    }

    const auto begin = std::chrono::steady_clock::now();
    const auto duration = std::chrono::duration<double>(parsed.duration);
    const auto period = std::chrono::duration<double>(1.0 / parsed.rate_hz);
    std::size_t step = 0;
    for (;;) {
        if (client_example::stopRequested()) return client_example::kInterrupted;
        const auto elapsed = std::chrono::steady_clock::now() - begin;
        const double alpha = std::min(
            1.0,
            std::chrono::duration<double>(elapsed).count() / parsed.duration);
        sdk::ControlGroupCommand command;
        for (std::size_t target_index = 0;
             target_index < parsed.targets.size(); ++target_index) {
            auto position = starts[target_index];
            for (std::size_t joint = 0; joint < position.size(); ++joint) {
                position[joint] += alpha *
                    (parsed.targets[target_index].position[joint] - position[joint]);
            }
            command.joint_cmd.push_back(makeJointCommand(
                parsed.targets[target_index].group_name, std::move(position)));
        }
        const auto result = client.publishControlGroupCommand(command);
        if (!result.success()) {
            client_example::printResult("publishControlGroupCommand", result);
            return client_example::kRuntimeFailure;
        }
        if (elapsed >= duration) return client_example::kSuccess;
        ++step;
        const auto deadline = begin +
            std::chrono::duration_cast<std::chrono::steady_clock::duration>(
                period * static_cast<double>(step));
        const auto now = std::chrono::steady_clock::now();
        if (deadline > now) {
            client_example::sleepInterruptibly(
                std::chrono::duration_cast<std::chrono::milliseconds>(deadline - now));
        }
    }
}

int execute(sdk::AuroraClient& client, const StateCommand& command) {
    if (command.action == StateCommand::Action::Request) {
        const auto result = client.requestFsmState(command.state_id);
        client_example::printResult("requestFsmState", result, std::cout);
        return result.success() ? client_example::kSuccess : client_example::kRuntimeFailure;
    }
    if (command.action == StateCommand::Action::Monitor) {
        return monitor<sdk::AuroraState>(
            client, command.interval, "getAuroraState",
            [](sdk::AuroraClient& value, sdk::AuroraState& sample) {
                return value.getAuroraState(sample);
            },
            printCurrentState);
    }
    sdk::AuroraState state;
    const auto result = client.getAuroraState(state);
    if (!result.success()) {
        client_example::printResult("getAuroraState", result);
        return client_example::kRuntimeFailure;
    }
    if (command.action == StateCommand::Action::Get) printCurrentState(state);
    else printAvailableStates(state);
    return client_example::kSuccess;
}

int execute(sdk::AuroraClient& client, const EstimatorCommand& command) {
    if (command.mode == ReadMode::Monitor) {
        return monitor<sdk::EstimatorData>(
            client, command.interval, "getEstimatorData",
            [](sdk::AuroraClient& value, sdk::EstimatorData& sample) {
                return value.getEstimatorData(sample);
            },
            printEstimator);
    }
    sdk::EstimatorData data;
    const auto result = client.getEstimatorData(data);
    if (!result.success()) {
        client_example::printResult("getEstimatorData", result);
        return client_example::kRuntimeFailure;
    }
    printEstimator(data);
    return client_example::kSuccess;
}

int execute(sdk::AuroraClient& client, const MotionCommand& command) {
    if (command.action == MotionCommand::Action::Monitor) {
        return monitor<sdk::MotionState>(
            client, command.interval, "getMotionState",
            [](sdk::AuroraClient& value, sdk::MotionState& sample) {
                return value.getMotionState(sample);
            },
            printMotionState);
    }
    if (command.action == MotionCommand::Action::Publish) {
        const auto lease = client.registerLease({sdk::AuroraResource::MotionCommand});
        if (!lease.success()) {
            client_example::printResult("registerLease", lease.operation);
            return client_example::kRuntimeFailure;
        }
        const auto result = client.publishMotionCommand(command.command);
        client_example::printResult("publishMotionCommand", result, std::cout);
        return result.success() ? client_example::kSuccess : client_example::kRuntimeFailure;
    }
    sdk::MotionState state;
    const auto result = client.getMotionState(state);
    if (!result.success()) {
        client_example::printResult("getMotionState", result);
        return client_example::kRuntimeFailure;
    }
    printMotionState(state);
    return client_example::kSuccess;
}

int execute(sdk::AuroraClient& client, const ControlGroupCommand& command) {
    if (command.action == ControlGroupCommand::Action::Monitor) {
        return monitor<sdk::ControlGroupState>(
            client, command.interval, "getControlGroupState",
            [](sdk::AuroraClient& value, sdk::ControlGroupState& sample) {
                return value.getControlGroupState(sample);
            },
            printControlGroupState);
    }
    if (command.action == ControlGroupCommand::Action::Position) {
        return setPositions(client, command);
    }
    sdk::ControlGroupState state;
    const auto result = client.getControlGroupState(state);
    if (!result.success()) {
        client_example::printResult("getControlGroupState", result);
        return client_example::kRuntimeFailure;
    }
    printControlGroupState(state);
    return client_example::kSuccess;
}

}  // namespace

int main(int argc, char** argv) {
    GlobalOptions global;
    std::vector<std::string> args;
    std::string error;
    if (!parseGlobalOptions(argc, argv, global, args, error)) {
        std::cerr << "error: " << error << "\n";
        printRootHelp(std::cerr);
        return client_example::kUsageError;
    }
    if (args.empty() || (args.size() == 1 && isHelp(args.front()))) {
        printRootHelp(std::cout);
        return client_example::kSuccess;
    }
    if (args.size() == 1 && args.front() == "--version") {
        std::cout << "aurora-client " << FOURIER_AURORA_SDK_VERSION << "\n";
        return client_example::kSuccess;
    }

    const std::string area = args.front();
    std::vector<std::string> area_args(args.begin() + 1, args.end());
    auto printAreaHelp = [&]() {
        if (area == "state") printStateHelp(std::cout);
        else if (area == "estimator") printEstimatorHelp(std::cout);
        else if (area == "motion") printMotionHelp(std::cout);
        else if (area == "control-group") printControlGroupHelp(std::cout);
        else printRootHelp(std::cout);
    };
    if (area_args.empty() || containsHelp(area_args)) {
        if (area != "state" && area != "estimator" && area != "motion" &&
            area != "control-group") {
            std::cerr << "error: unknown area: " << area << "\n";
            printRootHelp(std::cerr);
            return client_example::kUsageError;
        }
        if (area_args.empty() ||
            !printCommandHelp(area, area_args.front(), std::cout)) {
            printAreaHelp();
        }
        return client_example::kSuccess;
    }

    Command command = StateCommand{};
    bool valid = false;
    if (area == "state") {
        StateCommand parsed{};
        valid = parseState(area_args, parsed, error);
        command = parsed;
    } else if (area == "estimator") {
        EstimatorCommand parsed{};
        valid = parseEstimator(area_args, parsed, error);
        command = parsed;
    } else if (area == "motion") {
        MotionCommand parsed{};
        valid = parseMotion(area_args, parsed, error);
        command = parsed;
    } else if (area == "control-group") {
        ControlGroupCommand parsed{};
        valid = parseControlGroup(area_args, parsed, error);
        command = parsed;
    } else {
        error = "unknown area: " + area;
    }
    if (!valid) {
        std::cerr << "error: " << error << "\n";
        if (area == "state") printStateHelp(std::cerr);
        else if (area == "estimator") printEstimatorHelp(std::cerr);
        else if (area == "motion") printMotionHelp(std::cerr);
        else if (area == "control-group") printControlGroupHelp(std::cerr);
        else printRootHelp(std::cerr);
        return client_example::kUsageError;
    }

    client_example::installSignalHandlers();
    auto& client = sdk::AuroraClient::getInstance();
    try {
        const std::string source_name = global.source_name.has_value()
            ? *global.source_name : generateDefaultSourceName();
        auto options = client.config();
        options.domain_id = global.domain_id;
        options.source_name = source_name;
        options.enabled_endpoints = endpointsFor(command);
        client.configure(options);
        std::cerr << "domain_id=" << global.domain_id
                  << " source_name=" << source_name << "\n";
        client.start();
        if (!client.waitForEndpoints(kEndpointTimeout)) {
            std::cerr << "Timed out after 5s waiting for required endpoints.\n";
            return client_example::stopClient(client, client_example::kRuntimeFailure);
        }
        int exit_code = client_example::stopRequested()
            ? client_example::kInterrupted
            : std::visit([&](const auto& selected) {
                return execute(client, selected);
            }, command);
        return client_example::stopClient(client, exit_code);
    } catch (const std::exception& caught) {
        std::cerr << "aurora-client failed: " << caught.what() << "\n";
        return client_example::stopClient(client, client_example::kRuntimeFailure);
    }
}
