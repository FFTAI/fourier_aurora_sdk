#include "fourier_aurora_sdk/transport/fourierdds_transport.hpp"

#include <algorithm>
#include <array>
#include <chrono>
#include <condition_variable>
#include <exception>
#include <future>
#include <iostream>
#include <memory>
#include <mutex>
#include <stdexcept>
#include <string>
#include <type_traits>
#include <utility>
#include <vector>

#include "fourier_aurora_sdk/errors.hpp"
#include "internal/callback_context.hpp"
#include "internal/connection_options.hpp"
#include "internal/teardown_coordinator.hpp"
#include "internal/transport.hpp"

#if defined(FOURIER_AURORA_SDK_ENABLE_DDS_TRANSPORT)
#include "fourier_dds/dds_client.hpp"
#include "fourier_dds/dds_node.hpp"
#include "fourier_dds/dds_publisher.hpp"
#include "fourier_dds/dds_subscription.hpp"

#include "AuroraState/AuroraState.hpp"
#include "AuroraState/AuroraStatePubSubTypes.hpp"
#include "ErrorCodes/ErrorCodes.hpp"
#include "ErrorCodes/ErrorCodesPubSubTypes.hpp"
#include "EstimatorData/EstimatorData.hpp"
#include "EstimatorData/EstimatorDataPubSubTypes.hpp"
#include "RobotControlGroupCmd/RobotControlGroupCmd.hpp"
#include "RobotControlGroupCmd/RobotControlGroupCmdPubSubTypes.hpp"
#include "RobotControlGroupState/RobotControlGroupState.hpp"
#include "RobotControlGroupState/RobotControlGroupStatePubSubTypes.hpp"
#include "RobotMotionCmd/RobotMotionCmd.hpp"
#include "RobotMotionCmd/RobotMotionCmdPubSubTypes.hpp"
#include "RobotMotionState/RobotMotionState.hpp"
#include "RobotMotionState/RobotMotionStatePubSubTypes.hpp"
#include "SetInt/SetInt.hpp"
#include "SetInt/SetIntPubSubTypes.hpp"
#include "SubControllerCmd/SubControllerCmd.hpp"
#include "SubControllerCmd/SubControllerCmdPubSubTypes.hpp"
#endif

namespace fourier::aurora_sdk {

#if defined(FOURIER_AURORA_SDK_ENABLE_DDS_TRANSPORT)
namespace dds_msg = fourier_msgs::msg;
namespace dds_srv = fourier_msgs::srv;

namespace {

template <size_t N>
std::array<double, N> toArray(const std::vector<double>& values, const char* field_name) {
    if (values.size() != N) {
        throw InvalidArgumentError(
            std::string(field_name) + " expected length " + std::to_string(N) +
            ", got " + std::to_string(values.size()));
    }
    std::array<double, N> out{};
    std::copy(values.begin(), values.end(), out.begin());
    return out;
}

template <size_t N>
std::array<double, N> toArrayOrDefault(const std::vector<double>& values, const char* field_name) {
    if (values.empty()) {
        return {};
    }
    return toArray<N>(values, field_name);
}

template <size_t N>
std::vector<double> toVector(const std::array<double, N>& values) {
    return std::vector<double>(values.begin(), values.end());
}

std::chrono::system_clock::time_point toTimePoint(const builtin_interfaces::msg::Time& stamp) {
    const auto seconds = std::chrono::seconds(stamp.sec());
    const auto nanoseconds = std::chrono::nanoseconds(stamp.nanosec());
    return std::chrono::system_clock::time_point(seconds + nanoseconds);
}

builtin_interfaces::msg::Time toDdsTime(std::chrono::system_clock::time_point time) {
    const auto since_epoch = time.time_since_epoch();
    const auto seconds = std::chrono::duration_cast<std::chrono::seconds>(since_epoch);
    const auto nanoseconds =
        std::chrono::duration_cast<std::chrono::nanoseconds>(since_epoch - seconds);
    builtin_interfaces::msg::Time stamp;
    stamp.sec(static_cast<int32_t>(seconds.count()));
    stamp.nanosec(static_cast<uint32_t>(nanoseconds.count()));
    return stamp;
}

MessageHeader convertHeader(
    const dds_msg::TracerHeader& header,
    std::chrono::system_clock::time_point received_time) {
    MessageHeader out;
    out.send_time = toTimePoint(header.stamp());
    out.frame_id = header.frame_id();
    out.received_time = received_time;
    return out;
}

MessageHeader convertHeader(
    const std_msgs::msg::Header& header,
    std::chrono::system_clock::time_point received_time) {
    MessageHeader out;
    out.send_time = toTimePoint(header.stamp());
    out.frame_id = header.frame_id();
    out.received_time = received_time;
    return out;
}

dds_msg::TracerHeader makeTracerHeader(std::chrono::system_clock::time_point send_time) {
    dds_msg::TracerHeader header;
    header.stamp(toDdsTime(send_time));
    header.frame_id("");
    return header;
}

StateInfo convertStateInfo(const dds_msg::StateInfo& state) {
    StateInfo out;
    out.id = state.id();
    out.version = state.version();
    out.name = state.name();
    out.description = state.description();
    out.active = state.active();
    return out;
}

AuroraState convertAuroraState(const dds_msg::AuroraState& msg) {
    const auto received_time = std::chrono::system_clock::now();
    AuroraState out;
    out.header = convertHeader(msg.header(), received_time);
    out.robot_info.robot_type = msg.robot_info().robot_type();
    out.robot_info.hardware_type = msg.robot_info().hardware_type();
    out.robot_info.end_effector_type = msg.robot_info().end_effector_type();
    out.switch_status = msg.switch_status();
    out.current_state = convertStateInfo(msg.current_state());
    out.available_states.reserve(msg.available_states().size());
    for (const auto& state : msg.available_states()) {
        out.available_states.push_back(convertStateInfo(state));
    }
    return out;
}

MotionState convertMotionState(const dds_msg::RobotMotionState& msg) {
    const auto received_time = std::chrono::system_clock::now();
    MotionState out;
    out.header = convertHeader(msg.header(), received_time);
    out.position = toArray<3>(msg.position(), "RobotMotionState.position");
    out.position_cmd = toArray<3>(msg.position_cmd(), "RobotMotionState.position_cmd");
    out.lin_velocity = toArray<3>(msg.lin_velocity(), "RobotMotionState.lin_velocity");
    out.lin_velocity_cmd = toArray<3>(msg.lin_velocity_cmd(), "RobotMotionState.lin_velocity_cmd");
    out.attitude = toArray<3>(msg.attitude(), "RobotMotionState.attitude");
    out.attitude_cmd = toArray<3>(msg.attitude_cmd(), "RobotMotionState.attitude_cmd");
    out.ang_velocity = toArray<3>(msg.ang_velocity(), "RobotMotionState.ang_velocity");
    out.ang_velocity_cmd = toArray<3>(msg.ang_velocity_cmd(), "RobotMotionState.ang_velocity_cmd");
    out.delta_height = msg.delta_height();
    out.delta_height_cmd = msg.delta_height_cmd();
    out.progress = msg.progress();
    out.stable_level = msg.stable_level();
    return out;
}

ControlGroupStateEntry convertControlGroupEntry(const dds_msg::ControlGroupState& entry) {
    ControlGroupStateEntry out;
    out.group_name = entry.group_name();
    out.joint_position = entry.joint_position();
    out.joint_velocity = entry.joint_velocity();
    out.joint_effort = entry.joint_effort();
    return out;
}

CartesianStateEntry convertCartesianEntry(const dds_msg::CartesianState& entry) {
    CartesianStateEntry out;
    out.link_name = entry.link_name();
    out.pose = toArrayOrDefault<7>(entry.pose(), "CartesianState.pose");
    out.twist = toArrayOrDefault<6>(entry.twist(), "CartesianState.twist");
    out.wrench = toArrayOrDefault<6>(entry.wrench(), "CartesianState.wrench");
    return out;
}

ControlGroupState convertControlGroupState(const dds_msg::RobotControlGroupState& msg) {
    const auto received_time = std::chrono::system_clock::now();
    ControlGroupState out;
    out.header = convertHeader(msg.header(), received_time);
    out.joint_state.reserve(msg.joint_state().size());
    for (const auto& entry : msg.joint_state()) {
        out.joint_state.push_back(convertControlGroupEntry(entry));
    }
    out.joint_cmd.reserve(msg.joint_cmd().size());
    for (const auto& entry : msg.joint_cmd()) {
        out.joint_cmd.push_back(convertControlGroupEntry(entry));
    }
    out.cartesian_state.reserve(msg.cartesian_state().size());
    for (const auto& entry : msg.cartesian_state()) {
        out.cartesian_state.push_back(convertCartesianEntry(entry));
    }
    out.cartesian_cmd.reserve(msg.cartesian_cmd().size());
    for (const auto& entry : msg.cartesian_cmd()) {
        out.cartesian_cmd.push_back(convertCartesianEntry(entry));
    }
    return out;
}

EstimatorData convertEstimatorData(const dds_msg::EstimatorData& msg) {
    const auto received_time = std::chrono::system_clock::now();
    EstimatorData out;
    out.header = convertHeader(msg.header(), received_time);
    out.quat_xyzw = toArray<4>(msg.quat_xyzw(), "EstimatorData.quat_xyzw");
    out.quat_wxyz = toArray<4>(msg.quat_wxyz(), "EstimatorData.quat_wxyz");
    out.rpy = toArray<3>(msg.rpy(), "EstimatorData.rpy");
    out.omega_W = toArray<3>(msg.omega_W(), "EstimatorData.omega_W");
    out.acc_W = toArray<3>(msg.acc_W(), "EstimatorData.acc_W");
    out.omega_B = toArray<3>(msg.omega_B(), "EstimatorData.omega_B");
    out.acc_B = toArray<3>(msg.acc_B(), "EstimatorData.acc_B");
    out.vel_W = toArray<3>(msg.vel_W(), "EstimatorData.vel_W");
    out.pos_W = toArray<3>(msg.pos_W(), "EstimatorData.pos_W");
    out.vel_B = toArray<3>(msg.vel_B(), "EstimatorData.vel_B");
    out.contact_force = msg.contact_force();
    out.contact_prob = msg.contact_prob();
    out.stable_level = msg.stable_level();
    return out;
}

ErrorCodes convertErrorCodes(const dds_msg::ErrorCodes& msg) {
    const auto received_time = std::chrono::system_clock::now();
    ErrorCodes out;
    out.header = convertHeader(msg.header(), received_time);
    out.source = msg.source();
    out.error_codes.reserve(msg.error_codes().size());
    for (const auto& code : msg.error_codes()) {
        out.error_codes.push_back(ErrorCodeEntry{code.high32(), code.low32()});
    }
    return out;
}

dds_msg::RobotMotionCmd convertMotionCommand(const MotionCommand& command) {
    dds_msg::RobotMotionCmd msg;
    msg.header(makeTracerHeader(command.header.send_time));
    msg.position(toVector(command.position));
    msg.lin_velocity(toVector(command.lin_velocity));
    msg.attitude(toVector(command.attitude));
    msg.ang_velocity(toVector(command.ang_velocity));
    msg.delta_height(command.delta_height);
    msg.gait_type(command.gait_type);
    msg.skills(std::vector<uint8_t>(command.skills.begin(), command.skills.end()));
    msg.duration(command.duration);
    return msg;
}

dds_msg::RobotControlGroupCmd convertControlGroupCommand(const ControlGroupCommand& command) {
    dds_msg::RobotControlGroupCmd msg;
    msg.header(makeTracerHeader(command.header.send_time));
    std::vector<dds_msg::JointCmd> joint_cmds;
    joint_cmds.reserve(command.joint_cmd.size());
    for (const auto& command_entry : command.joint_cmd) {
        dds_msg::JointCmd entry;
        entry.group_name(command_entry.group_name);
        entry.control_type(static_cast<uint8_t>(command_entry.control_type));
        entry.motor_mode(static_cast<uint8_t>(command_entry.motor_mode));
        entry.position(command_entry.position);
        entry.velocity(command_entry.velocity);
        entry.effort(command_entry.effort);
        joint_cmds.push_back(std::move(entry));
    }
    msg.joint_cmd(std::move(joint_cmds));

    msg.cartesian_cmd({});
    return msg;
}

dds_msg::SubControllerCmd convertSubControllerCommand(const SubControllerCommand& command) {
    dds_msg::SubControllerCmd msg;
    msg.header(makeTracerHeader(command.header.send_time));
    msg.reset(command.reset);

    std::vector<dds_msg::ControllerParam> params;
    params.reserve(command.controller_params.size());
    for (const auto& controller : command.controller_params) {
        dds_msg::ControllerParam controller_msg;
        controller_msg.controller_name(controller.controller_name);
        controller_msg.rate(controller.rate);
        std::vector<dds_msg::GroupOverrideParam> overrides;
        overrides.reserve(controller.group_override_params.size());
        for (const auto& override_param : controller.group_override_params) {
            dds_msg::GroupOverrideParam override_msg;
            override_msg.group_name(override_param.group_name);
            override_msg.control_param(override_param.control_param);
            overrides.push_back(std::move(override_msg));
        }
        controller_msg.group_override_params(std::move(overrides));
        params.push_back(std::move(controller_msg));
    }
    msg.controller_params(std::move(params));
    return msg;
}

}  // namespace

#endif

struct FourierDdsTransport::InitialHandlers {
    detail::TransportHandlers callbacks;
    detail::TransportTerminalObserver terminal_observer;
    bool callbacks_are_user = true;
};

#if defined(FOURIER_AURORA_SDK_ENABLE_DDS_TRANSPORT)

namespace {

OperationResult makeResult(uint32_t code, const std::string& detail = {}) {
    OperationResult result;
    result.code = code;
    result.message = std::string(defaultResultMessage(code));
    if (!detail.empty()) {
        result.message += ": ";
        result.message += detail;
    }
    return result;
}

const char* endpointName(Endpoint endpoint) {
    switch (endpoint) {
        case Endpoint::FsmService:
            return "fsm_service";
        case Endpoint::MotionCommandPublisher:
            return "motion_command_publisher";
        case Endpoint::ControlGroupCommandPublisher:
            return "control_group_command_publisher";
        case Endpoint::SubControllerCommandPublisher:
            return "sub_controller_command_publisher";
        case Endpoint::AuroraStateSubscriber:
            return "aurora_state_subscriber";
        case Endpoint::MotionStateSubscriber:
            return "motion_state_subscriber";
        case Endpoint::ControlGroupStateSubscriber:
            return "control_group_state_subscriber";
        case Endpoint::EstimatorDataSubscriber:
            return "estimator_data_subscriber";
        case Endpoint::ErrorCodesSubscriber:
            return "error_codes_subscriber";
    }
    return "invalid_endpoint";
}

EndpointStatus& endpointStatus(
    EndpointMatchStatus& status,
    Endpoint endpoint) {
    switch (endpoint) {
        case Endpoint::FsmService:
            return status.fsm_service;
        case Endpoint::MotionCommandPublisher:
            return status.motion_command_publisher;
        case Endpoint::ControlGroupCommandPublisher:
            return status.control_group_command_publisher;
        case Endpoint::SubControllerCommandPublisher:
            return status.sub_controller_command_publisher;
        case Endpoint::AuroraStateSubscriber:
            return status.aurora_state_subscriber;
        case Endpoint::MotionStateSubscriber:
            return status.motion_state_subscriber;
        case Endpoint::ControlGroupStateSubscriber:
            return status.control_group_state_subscriber;
        case Endpoint::EstimatorDataSubscriber:
            return status.estimator_data_subscriber;
        case Endpoint::ErrorCodesSubscriber:
            return status.error_codes_subscriber;
    }
    throw InvalidArgumentError("invalid Endpoint value");
}

std::shared_future<void> readyFuture() {
    std::promise<void> promise;
    promise.set_value();
    return promise.get_future().share();
}

}  // namespace

struct FourierDdsTransport::Impl
    : public std::enable_shared_from_this<FourierDdsTransport::Impl> {
    using Node = fourier_dds::DdsNode;
    using MotionCommandPublisher = fourier_dds::DdsPublisher<dds_msg::RobotMotionCmdPubSubType>;
    using ControlGroupCommandPublisher = fourier_dds::DdsPublisher<dds_msg::RobotControlGroupCmdPubSubType>;
    using SubControllerCommandPublisher = fourier_dds::DdsPublisher<dds_msg::SubControllerCmdPubSubType>;
    using AuroraStateSubscription = fourier_dds::DdsSubscription<dds_msg::AuroraStatePubSubType>;
    using MotionStateSubscription = fourier_dds::DdsSubscription<dds_msg::RobotMotionStatePubSubType>;
    using ControlGroupStateSubscription =
        fourier_dds::DdsSubscription<dds_msg::RobotControlGroupStatePubSubType>;
    using EstimatorDataSubscription = fourier_dds::DdsSubscription<dds_msg::EstimatorDataPubSubType>;
    using ErrorCodesSubscription = fourier_dds::DdsSubscription<dds_msg::ErrorCodesPubSubType>;
    using FsmClient = fourier_dds::DdsClient<dds_srv::SetInt_RequestPubSubType, dds_srv::SetInt_ResponsePubSubType>;

    class OperationAdmission {
    public:
        OperationAdmission() = default;

        explicit OperationAdmission(std::shared_ptr<Impl> session)
            : session_(std::move(session)) {}

        OperationAdmission(OperationAdmission&& other) noexcept
            : session_(std::move(other.session_)) {}

        OperationAdmission& operator=(OperationAdmission&& other) noexcept {
            if (this != &other) {
                release();
                session_ = std::move(other.session_);
            }
            return *this;
        }

        ~OperationAdmission() {
            release();
        }

        OperationAdmission(const OperationAdmission&) = delete;
        OperationAdmission& operator=(const OperationAdmission&) = delete;

        explicit operator bool() const noexcept {
            return static_cast<bool>(session_);
        }

        void release() noexcept {
            if (session_) {
                session_->finishOperation();
                session_.reset();
            }
        }

    private:
        std::shared_ptr<Impl> session_;
    };

    class CallbackAdmission {
    public:
        explicit CallbackAdmission(std::shared_ptr<Impl> session)
            : session_(std::move(session)) {}

        ~CallbackAdmission() {
            if (session_) {
                session_->finishCallback();
            }
        }

        CallbackAdmission(const CallbackAdmission&) = delete;
        CallbackAdmission& operator=(const CallbackAdmission&) = delete;

    private:
        std::shared_ptr<Impl> session_;
    };

    struct AsyncLogSlot {
        std::chrono::steady_clock::time_point last_log{};
        uint32_t suppressed = 0;
        bool initialized = false;
    };

    static std::shared_ptr<Impl> create(
        uint32_t domain_id,
        const ConnectionOptions& public_options,
        InitialHandlers initial_handlers) {
        auto& coordinator = detail::TeardownCoordinator::instance();
        coordinator.ensureWorker();

        auto session = std::shared_ptr<Impl>(
            new Impl(
                detail::normalizeConnectionOptions(public_options),
                std::move(initial_handlers)));
        try {
            session->initialize(domain_id);
        } catch (const std::exception& error) {
            try {
                session->requestStop().get();
            } catch (...) {
            }
            throw ConnectionError(
                std::string("failed to initialize FourierDDS transport: ") +
                error.what());
        } catch (...) {
            try {
                session->requestStop().get();
            } catch (...) {
            }
            throw ConnectionError("failed to initialize FourierDDS transport");
        }
        return session;
    }

    Impl(
        detail::NormalizedConnectionOptions normalized_options,
        InitialHandlers initial_handlers)
        : options(std::move(normalized_options)),
          aurora_state_handler(
              std::move(initial_handlers.callbacks.aurora_state)),
          motion_state_handler(
              std::move(initial_handlers.callbacks.motion_state)),
          control_group_state_handler(
              std::move(initial_handlers.callbacks.control_group_state)),
          estimator_data_handler(
              std::move(initial_handlers.callbacks.estimator_data)),
          error_codes_handler(
              std::move(initial_handlers.callbacks.error_codes)),
          terminal_observer(std::move(initial_handlers.terminal_observer)),
          callbacks_are_user(initial_handlers.callbacks_are_user) {}

    void initialize(uint32_t domain_id) {
        const auto& public_options = options.public_options;
        const bool node_uses_namespace =
            options.has_namespace_override ? false : public_options.use_namespace;
        node = std::make_shared<Node>(
            domain_id,
            public_options.dds_mode,
            options.has_ros_compatible_override
                ? std::optional<bool>(options.is_ros_compatible)
                : std::nullopt,
            node_uses_namespace,
            options.has_discovery_server_override
                ? std::optional<bool>(options.use_discovery_server)
                : std::nullopt);
        if (options.has_namespace_override) {
            node->setNamespace(options.namespace_name);
        }

        if (options.endpointEnabled(Endpoint::MotionCommandPublisher)) {
            motion_command_pub =
                std::make_unique<MotionCommandPublisher>(node, "robot_motion_cmd");
        }
        if (options.endpointEnabled(
                Endpoint::ControlGroupCommandPublisher)) {
            control_group_command_pub =
                std::make_unique<ControlGroupCommandPublisher>(
                    node, "robot_control_group_cmd");
        }
        if (options.endpointEnabled(
                Endpoint::SubControllerCommandPublisher)) {
            sub_controller_command_pub =
                std::make_unique<SubControllerCommandPublisher>(
                    node, "sub_controller_cmd");
        }

        const std::weak_ptr<Impl> weak = shared_from_this();
        if (options.endpointEnabled(Endpoint::AuroraStateSubscriber)) {
            aurora_state_sub = std::make_unique<AuroraStateSubscription>(
                node,
                "aurora_state",
                [weak](const dds_msg::AuroraState& message) {
                    if (const auto session = weak.lock()) {
                        session->dispatch(
                            Endpoint::AuroraStateSubscriber,
                            &Impl::aurora_state_handler,
                            [&message] { return convertAuroraState(message); });
                    }
                },
                false,
                fourier_dds::SubscriptionDurability::Volatile);
        }
        if (options.endpointEnabled(Endpoint::MotionStateSubscriber)) {
            motion_state_sub = std::make_unique<MotionStateSubscription>(
                node,
                "robot_motion_state",
                [weak](const dds_msg::RobotMotionState& message) {
                    if (const auto session = weak.lock()) {
                        session->dispatch(
                            Endpoint::MotionStateSubscriber,
                            &Impl::motion_state_handler,
                            [&message] { return convertMotionState(message); });
                    }
                },
                false,
                fourier_dds::SubscriptionDurability::Volatile);
        }
        if (options.endpointEnabled(
                Endpoint::ControlGroupStateSubscriber)) {
            control_group_state_sub =
                std::make_unique<ControlGroupStateSubscription>(
                    node,
                    "robot_control_group_state",
                    [weak](const dds_msg::RobotControlGroupState& message) {
                        if (const auto session = weak.lock()) {
                            session->dispatch(
                                Endpoint::ControlGroupStateSubscriber,
                                &Impl::control_group_state_handler,
                                [&message] {
                                    return convertControlGroupState(message);
                                });
                        }
                    },
                    false,
                    fourier_dds::SubscriptionDurability::Volatile);
        }
        if (options.endpointEnabled(Endpoint::EstimatorDataSubscriber)) {
            estimator_data_sub = std::make_unique<EstimatorDataSubscription>(
                node,
                "estimator_data",
                [weak](const dds_msg::EstimatorData& message) {
                    if (const auto session = weak.lock()) {
                        session->dispatch(
                            Endpoint::EstimatorDataSubscriber,
                            &Impl::estimator_data_handler,
                            [&message] { return convertEstimatorData(message); });
                    }
                },
                false,
                fourier_dds::SubscriptionDurability::Volatile);
        }
        if (options.endpointEnabled(Endpoint::ErrorCodesSubscriber)) {
            error_codes_sub = std::make_unique<ErrorCodesSubscription>(
                node,
                "error_codes_pub",
                [weak](const dds_msg::ErrorCodes& message) {
                    if (const auto session = weak.lock()) {
                        session->dispatch(
                            Endpoint::ErrorCodesSubscriber,
                            &Impl::error_codes_handler,
                            [&message] { return convertErrorCodes(message); });
                    }
                },
                false,
                fourier_dds::SubscriptionDurability::Volatile);
        }
        if (options.endpointEnabled(Endpoint::FsmService)) {
            fsm_service = std::make_unique<FsmClient>(
                node, "whole_body_fsm_state_change_service");
        }
    }

    template <typename Handler, typename Converter>
    void dispatch(
        Endpoint endpoint,
        Handler Impl::* handler_member,
        Converter&& converter) {
        Handler handler;
        bool is_user_callback = false;
        {
            std::lock_guard<std::mutex> lock(mutex);
            if (!accepting) {
                return;
            }
            ++active_callbacks;
            handler = this->*handler_member;
            is_user_callback = callbacks_are_user;
        }
        CallbackAdmission admission(shared_from_this());
        if (!handler) {
            return;
        }

        using Value = std::decay_t<decltype(converter())>;
        Value value;
        try {
            value = converter();
        } catch (const std::exception& error) {
            logAsyncFailure(endpoint, 0, error.what());
            return;
        } catch (...) {
            logAsyncFailure(endpoint, 0, "non-standard conversion failure");
            return;
        }

        try {
            if (is_user_callback) {
                detail::UserCallbackScope callback_scope;
                handler(std::move(value));
            } else {
                handler(std::move(value));
            }
        } catch (const std::exception& error) {
            logAsyncFailure(endpoint, 1, error.what());
        } catch (...) {
            logAsyncFailure(endpoint, 1, "non-standard callback exception");
        }
    }

    void logAsyncFailure(
        Endpoint endpoint,
        std::size_t category,
        const std::string& message) {
        const auto index = detail::endpointIndex(endpoint) * 2 + category;
        const auto now = std::chrono::steady_clock::now();
        uint32_t suppressed = 0;
        bool should_log = false;
        {
            std::lock_guard<std::mutex> lock(log_mutex);
            auto& slot = async_log_slots[index];
            if (!slot.initialized ||
                now - slot.last_log >= std::chrono::seconds(5)) {
                should_log = true;
                suppressed = slot.suppressed;
                slot.suppressed = 0;
                slot.last_log = now;
                slot.initialized = true;
            } else {
                ++slot.suppressed;
            }
        }
        if (should_log) {
            std::cerr << "[fourier_aurora_sdk] "
                      << (category == 0 ? "sample conversion" : "callback")
                      << " failure on " << endpointName(endpoint) << ": "
                      << message;
            if (suppressed != 0) {
                std::cerr << " (" << suppressed
                          << " similar failures suppressed)";
            }
            std::cerr << std::endl;
        }
    }

    OperationAdmission beginOperation(
        Endpoint endpoint,
        OperationResult& failure) {
        std::lock_guard<std::mutex> lock(mutex);
        if (!options.endpointEnabled(endpoint)) {
            failure = makeResult(result_codes::ENDPOINT_DISABLED);
            return {};
        }
        if (!accepting) {
            failure = makeResult(result_codes::CLIENT_NOT_RUNNING);
            return {};
        }
        ++active_operations;
        return OperationAdmission(shared_from_this());
    }

    void finishOperation() noexcept {
        std::lock_guard<std::mutex> lock(mutex);
        if (active_operations != 0) {
            --active_operations;
        }
        drained.notify_all();
    }

    void finishCallback() noexcept {
        std::lock_guard<std::mutex> lock(mutex);
        if (active_callbacks != 0) {
            --active_callbacks;
        }
        drained.notify_all();
    }

    [[noreturn]] void fatalInvariant(
        OperationAdmission& admission,
        const std::string& message) {
        std::cerr << "[fourier_aurora_sdk] fatal transport failure: "
                  << message << std::endl;
        admission.release();
        auto completion = requestStop();
        if (!detail::inUserCallback()) {
            try {
                completion.get();
            } catch (const std::exception& error) {
                throw ConnectionError(
                    message + "; teardown failed: " + error.what());
            }
        }
        throw ConnectionError(message);
    }

    template <typename Publisher, typename Converter>
    OperationResult publish(
        Endpoint endpoint,
        Publisher* publisher,
        Converter&& converter) {
        OperationResult failure;
        auto admission = beginOperation(endpoint, failure);
        if (!admission) {
            return failure;
        }
        if (publisher == nullptr) {
            fatalInvariant(admission, "configured publisher is unavailable");
        }
        if (!matched(endpoint)) {
            return makeResult(result_codes::ENDPOINT_UNMATCHED);
        }

        typename Publisher::MSG_TYPE message;
        try {
            message = converter();
        } catch (const std::exception& error) {
            return makeResult(result_codes::CLIENT_ERROR, error.what());
        } catch (...) {
            return makeResult(
                result_codes::CLIENT_ERROR,
                "non-standard command conversion failure");
        }

        try {
            if (!publisher->publish(message)) {
                return makeResult(result_codes::PUBLISH_FAILED);
            }
            return makeResult(result_codes::SUCCESS);
        } catch (const std::exception& error) {
            return makeResult(result_codes::TRANSPORT_ERROR, error.what());
        } catch (...) {
            return makeResult(
                result_codes::TRANSPORT_ERROR,
                "non-standard DDS publish failure");
        }
    }

    OperationResult requestFsmState(uint32_t state_id) {
        OperationResult failure;
        auto admission = beginOperation(Endpoint::FsmService, failure);
        if (!admission) {
            return failure;
        }
        if (fsm_service == nullptr) {
            fatalInvariant(admission, "configured FSM service is unavailable");
        }
        if (!matched(Endpoint::FsmService)) {
            return makeResult(result_codes::ENDPOINT_UNMATCHED);
        }

        dds_srv::SetInt_Request request;
        request.header(makeTracerHeader(std::chrono::system_clock::now()));
        request.data(static_cast<uint64_t>(state_id));
        dds_srv::SetInt_Response response;
        const auto start = std::chrono::steady_clock::now();
        try {
            const bool received = fsm_service->send_request(
                request,
                response,
                options.public_options.service_timeout_ms);
            if (!received) {
                if (!matched(Endpoint::FsmService)) {
                    return makeResult(result_codes::ENDPOINT_UNMATCHED);
                }
                const auto elapsed = std::chrono::steady_clock::now() - start;
                if (options.public_options.service_timeout_ms == 0 ||
                    elapsed >= std::chrono::milliseconds(
                                   options.public_options.service_timeout_ms)) {
                    return makeResult(result_codes::SERVICE_TIMEOUT);
                }
                return makeResult(result_codes::TRANSPORT_ERROR);
            }
        } catch (const std::exception& error) {
            if (!matched(Endpoint::FsmService)) {
                return makeResult(result_codes::ENDPOINT_UNMATCHED);
            }
            return makeResult(result_codes::TRANSPORT_ERROR, error.what());
        } catch (...) {
            return makeResult(
                result_codes::TRANSPORT_ERROR,
                "non-standard DDS service failure");
        }

        const auto wire_result = response.result();
        if (wire_result < 0) {
            return makeResult(result_codes::INVALID_AURORA_RESPONSE);
        }
        return OperationResult{
            static_cast<uint32_t>(wire_result),
            response.message(),
        };
    }

    bool matched(Endpoint endpoint) const {
        switch (endpoint) {
            case Endpoint::FsmService:
                return fsm_service && fsm_service->is_server_matched();
            case Endpoint::MotionCommandPublisher:
                return motion_command_pub &&
                       motion_command_pub->get_matched() > 0;
            case Endpoint::ControlGroupCommandPublisher:
                return control_group_command_pub &&
                       control_group_command_pub->get_matched() > 0;
            case Endpoint::SubControllerCommandPublisher:
                return sub_controller_command_pub &&
                       sub_controller_command_pub->get_matched() > 0;
            case Endpoint::AuroraStateSubscriber:
                return aurora_state_sub && aurora_state_sub->get_matched() > 0;
            case Endpoint::MotionStateSubscriber:
                return motion_state_sub && motion_state_sub->get_matched() > 0;
            case Endpoint::ControlGroupStateSubscriber:
                return control_group_state_sub &&
                       control_group_state_sub->get_matched() > 0;
            case Endpoint::EstimatorDataSubscriber:
                return estimator_data_sub &&
                       estimator_data_sub->get_matched() > 0;
            case Endpoint::ErrorCodesSubscriber:
                return error_codes_sub && error_codes_sub->get_matched() > 0;
        }
        throw InvalidArgumentError("invalid Endpoint value");
    }

    EndpointMatchStatus status() const {
        EndpointMatchStatus status;
        std::lock_guard<std::mutex> lock(mutex);
        for (const auto endpoint : options.public_options.enabled_endpoints) {
            auto& endpoint_status = endpointStatus(status, endpoint);
            endpoint_status.enabled = true;
            endpoint_status.matched = accepting && matched(endpoint);
        }
        return status;
    }

    template <typename Handler>
    void setHandler(
        Endpoint endpoint,
        Handler Impl::* handler_member,
        Handler handler) {
        std::lock_guard<std::mutex> lock(mutex);
        if (!options.endpointEnabled(endpoint)) {
            throw InvalidStateError(
                std::string(endpointName(endpoint)) + " is disabled");
        }
        if (!accepting) {
            throw InvalidStateError("transport session is stopped");
        }
        this->*handler_member = std::move(handler);
    }

    std::shared_future<void> requestStop() {
        std::unique_lock<std::mutex> lock(mutex);
        drained.wait(lock, [this] { return !teardown_submitting; });
        if (teardown_completion.valid()) {
            return teardown_completion;
        }
        if (stopped) {
            return readyFuture();
        }

        accepting = false;
        teardown_submitting = true;
        const auto self = shared_from_this();
        lock.unlock();

        std::shared_future<void> completion;
        try {
            completion = detail::TeardownCoordinator::instance().submit(
                [self] { self->teardownNow(); });
        } catch (...) {
            lock.lock();
            teardown_submitting = false;
            drained.notify_all();
            throw;
        }

        lock.lock();
        teardown_completion = completion;
        teardown_submitting = false;
        drained.notify_all();
        return completion;
    }

    void teardownNow() {
        std::shared_ptr<Node> detached_node;
        std::unique_ptr<FsmClient> detached_fsm_service;
        std::unique_ptr<MotionCommandPublisher> detached_motion_command_pub;
        std::unique_ptr<ControlGroupCommandPublisher>
            detached_control_group_command_pub;
        std::unique_ptr<SubControllerCommandPublisher>
            detached_sub_controller_command_pub;
        std::unique_ptr<AuroraStateSubscription> detached_aurora_state_sub;
        std::unique_ptr<MotionStateSubscription> detached_motion_state_sub;
        std::unique_ptr<ControlGroupStateSubscription>
            detached_control_group_state_sub;
        std::unique_ptr<EstimatorDataSubscription> detached_estimator_data_sub;
        std::unique_ptr<ErrorCodesSubscription> detached_error_codes_sub;

        {
            std::unique_lock<std::mutex> lock(mutex);
            drained.wait(lock, [this] {
                return active_operations == 0 && active_callbacks == 0;
            });
            aurora_state_handler = nullptr;
            motion_state_handler = nullptr;
            control_group_state_handler = nullptr;
            estimator_data_handler = nullptr;
            error_codes_handler = nullptr;

            detached_error_codes_sub = std::move(error_codes_sub);
            detached_estimator_data_sub = std::move(estimator_data_sub);
            detached_control_group_state_sub =
                std::move(control_group_state_sub);
            detached_motion_state_sub = std::move(motion_state_sub);
            detached_aurora_state_sub = std::move(aurora_state_sub);
            detached_sub_controller_command_pub =
                std::move(sub_controller_command_pub);
            detached_control_group_command_pub =
                std::move(control_group_command_pub);
            detached_motion_command_pub = std::move(motion_command_pub);
            detached_fsm_service = std::move(fsm_service);
            detached_node = std::move(node);
        }

        detached_error_codes_sub.reset();
        detached_estimator_data_sub.reset();
        detached_control_group_state_sub.reset();
        detached_motion_state_sub.reset();
        detached_aurora_state_sub.reset();
        detached_sub_controller_command_pub.reset();
        detached_control_group_command_pub.reset();
        detached_motion_command_pub.reset();
        detached_fsm_service.reset();
        detached_node.reset();

        detail::TransportTerminalObserver observer;
        {
            std::lock_guard<std::mutex> lock(mutex);
            stopped = true;
            observer = std::move(terminal_observer);
            drained.notify_all();
        }
        if (observer) {
            try {
                observer();
            } catch (...) {
                std::cerr
                    << "[fourier_aurora_sdk] internal terminal observer failed"
                    << std::endl;
            }
        }
    }

    detail::NormalizedConnectionOptions options;
    mutable std::mutex mutex;
    mutable std::mutex log_mutex;
    std::condition_variable drained;
    bool accepting = true;
    bool stopped = false;
    bool teardown_submitting = false;
    std::size_t active_operations = 0;
    std::size_t active_callbacks = 0;
    std::shared_future<void> teardown_completion;
    std::array<AsyncLogSlot, detail::kEndpointCount * 2> async_log_slots{};

    std::shared_ptr<Node> node;
    std::unique_ptr<FsmClient> fsm_service;
    std::unique_ptr<MotionCommandPublisher> motion_command_pub;
    std::unique_ptr<ControlGroupCommandPublisher> control_group_command_pub;
    std::unique_ptr<SubControllerCommandPublisher> sub_controller_command_pub;
    std::unique_ptr<AuroraStateSubscription> aurora_state_sub;
    std::unique_ptr<MotionStateSubscription> motion_state_sub;
    std::unique_ptr<ControlGroupStateSubscription> control_group_state_sub;
    std::unique_ptr<EstimatorDataSubscription> estimator_data_sub;
    std::unique_ptr<ErrorCodesSubscription> error_codes_sub;

    AuroraStateHandler aurora_state_handler;
    MotionStateHandler motion_state_handler;
    ControlGroupStateHandler control_group_state_handler;
    EstimatorDataHandler estimator_data_handler;
    ErrorCodesHandler error_codes_handler;
    detail::TransportTerminalObserver terminal_observer;
    bool callbacks_are_user = true;
};

FourierDdsTransport::FourierDdsTransport(
    uint32_t domain_id,
    ConnectionOptions options)
    : FourierDdsTransport(
          domain_id,
          std::move(options),
          InitialHandlers{}) {}

FourierDdsTransport::FourierDdsTransport(
    uint32_t domain_id,
    ConnectionOptions options,
    InitialHandlers handlers)
    : impl_(Impl::create(domain_id, options, std::move(handlers))) {}

FourierDdsTransport::~FourierDdsTransport() noexcept {
    if (!impl_) {
        return;
    }
    try {
        if (detail::inUserCallback()) {
            std::cerr
                << "[fourier_aurora_sdk] FourierDdsTransport destroyed from "
                   "callback context; teardown was deferred"
                << std::endl;
            (void)impl_->requestStop();
        } else {
            impl_->requestStop().get();
        }
    } catch (const std::exception& error) {
        std::cerr << "[fourier_aurora_sdk] transport destructor cleanup failed: "
                  << error.what() << std::endl;
    } catch (...) {
        std::cerr << "[fourier_aurora_sdk] transport destructor cleanup failed"
                  << std::endl;
    }
    impl_.reset();
}

void FourierDdsTransport::setAuroraStateHandler(AuroraStateHandler handler) {
    impl_->setHandler(
        Endpoint::AuroraStateSubscriber,
        &Impl::aurora_state_handler,
        std::move(handler));
}

void FourierDdsTransport::setMotionStateHandler(MotionStateHandler handler) {
    impl_->setHandler(
        Endpoint::MotionStateSubscriber,
        &Impl::motion_state_handler,
        std::move(handler));
}

void FourierDdsTransport::setControlGroupStateHandler(ControlGroupStateHandler handler) {
    impl_->setHandler(
        Endpoint::ControlGroupStateSubscriber,
        &Impl::control_group_state_handler,
        std::move(handler));
}

void FourierDdsTransport::setEstimatorDataHandler(EstimatorDataHandler handler) {
    impl_->setHandler(
        Endpoint::EstimatorDataSubscriber,
        &Impl::estimator_data_handler,
        std::move(handler));
}

void FourierDdsTransport::setErrorCodesHandler(ErrorCodesHandler handler) {
    impl_->setHandler(
        Endpoint::ErrorCodesSubscriber,
        &Impl::error_codes_handler,
        std::move(handler));
}

OperationResult FourierDdsTransport::publishMotionCommand(
    const MotionCommand& command) {
    return impl_->publish(
        Endpoint::MotionCommandPublisher,
        impl_->motion_command_pub.get(),
        [&command] { return convertMotionCommand(command); });
}

OperationResult FourierDdsTransport::publishControlGroupCommand(
    const ControlGroupCommand& command) {
    return impl_->publish(
        Endpoint::ControlGroupCommandPublisher,
        impl_->control_group_command_pub.get(),
        [&command] { return convertControlGroupCommand(command); });
}

OperationResult FourierDdsTransport::publishSubControllerCommand(
    const SubControllerCommand& command) {
    return impl_->publish(
        Endpoint::SubControllerCommandPublisher,
        impl_->sub_controller_command_pub.get(),
        [&command] { return convertSubControllerCommand(command); });
}

OperationResult FourierDdsTransport::requestFsmState(uint32_t state_id) {
    return impl_->requestFsmState(state_id);
}

EndpointMatchStatus FourierDdsTransport::endpointMatchStatus() const {
    return impl_->status();
}

bool FourierDdsTransport::isFsmServiceMatched() const {
    return endpointMatchStatus().fsm_service.matched;
}

bool FourierDdsTransport::isMotionCommandPublisherMatched() const {
    return endpointMatchStatus().motion_command_publisher.matched;
}

bool FourierDdsTransport::isControlGroupCommandPublisherMatched() const {
    return endpointMatchStatus().control_group_command_publisher.matched;
}

bool FourierDdsTransport::isSubControllerCommandPublisherMatched() const {
    return endpointMatchStatus().sub_controller_command_publisher.matched;
}

bool FourierDdsTransport::isAuroraStateSubscriberMatched() const {
    return endpointMatchStatus().aurora_state_subscriber.matched;
}

bool FourierDdsTransport::isMotionStateSubscriberMatched() const {
    return endpointMatchStatus().motion_state_subscriber.matched;
}

bool FourierDdsTransport::isControlGroupStateSubscriberMatched() const {
    return endpointMatchStatus().control_group_state_subscriber.matched;
}

bool FourierDdsTransport::isEstimatorDataSubscriberMatched() const {
    return endpointMatchStatus().estimator_data_subscriber.matched;
}

bool FourierDdsTransport::isErrorCodesSubscriberMatched() const {
    return endpointMatchStatus().error_codes_subscriber.matched;
}

void FourierDdsTransport::stop() {
    if (detail::inUserCallback()) {
        throw InvalidStateError(
            "stop() must not be called from an SDK callback");
    }
    try {
        impl_->requestStop().get();
    } catch (const ConnectionError&) {
        throw;
    } catch (const std::exception& error) {
        throw ConnectionError(
            std::string("transport teardown failed: ") + error.what());
    } catch (...) {
        throw ConnectionError("transport teardown failed");
    }
}

#else

struct FourierDdsTransport::Impl {};

FourierDdsTransport::FourierDdsTransport(
    uint32_t domain_id,
    ConnectionOptions options)
    : FourierDdsTransport(
          domain_id,
          std::move(options),
          InitialHandlers{}) {}

FourierDdsTransport::FourierDdsTransport(
    uint32_t,
    ConnectionOptions options,
    InitialHandlers) {
    (void)detail::normalizeConnectionOptions(options);
    throw ConnectionError(
        "FourierDDS transport is disabled in this SDK build");
}

FourierDdsTransport::~FourierDdsTransport() noexcept = default;

void FourierDdsTransport::setAuroraStateHandler(AuroraStateHandler) {
    throw InvalidStateError("transport session is stopped");
}
void FourierDdsTransport::setMotionStateHandler(MotionStateHandler) {
    throw InvalidStateError("transport session is stopped");
}
void FourierDdsTransport::setControlGroupStateHandler(
    ControlGroupStateHandler) {
    throw InvalidStateError("transport session is stopped");
}
void FourierDdsTransport::setEstimatorDataHandler(EstimatorDataHandler) {
    throw InvalidStateError("transport session is stopped");
}
void FourierDdsTransport::setErrorCodesHandler(ErrorCodesHandler) {
    throw InvalidStateError("transport session is stopped");
}

OperationResult FourierDdsTransport::publishMotionCommand(
    const MotionCommand&) {
    return OperationResult{
        result_codes::CLIENT_NOT_RUNNING,
        std::string(defaultResultMessage(result_codes::CLIENT_NOT_RUNNING)),
    };
}
OperationResult FourierDdsTransport::publishControlGroupCommand(
    const ControlGroupCommand&) {
    return OperationResult{
        result_codes::CLIENT_NOT_RUNNING,
        std::string(defaultResultMessage(result_codes::CLIENT_NOT_RUNNING)),
    };
}
OperationResult FourierDdsTransport::publishSubControllerCommand(
    const SubControllerCommand&) {
    return OperationResult{
        result_codes::CLIENT_NOT_RUNNING,
        std::string(defaultResultMessage(result_codes::CLIENT_NOT_RUNNING)),
    };
}
OperationResult FourierDdsTransport::requestFsmState(uint32_t) {
    return OperationResult{
        result_codes::CLIENT_NOT_RUNNING,
        std::string(defaultResultMessage(result_codes::CLIENT_NOT_RUNNING)),
    };
}
EndpointMatchStatus FourierDdsTransport::endpointMatchStatus() const {
    return {};
}
bool FourierDdsTransport::isFsmServiceMatched() const {
    return false;
}
bool FourierDdsTransport::isMotionCommandPublisherMatched() const {
    return false;
}
bool FourierDdsTransport::isControlGroupCommandPublisherMatched() const {
    return false;
}
bool FourierDdsTransport::isSubControllerCommandPublisherMatched() const {
    return false;
}
bool FourierDdsTransport::isAuroraStateSubscriberMatched() const {
    return false;
}
bool FourierDdsTransport::isMotionStateSubscriberMatched() const {
    return false;
}
bool FourierDdsTransport::isControlGroupStateSubscriberMatched() const {
    return false;
}
bool FourierDdsTransport::isEstimatorDataSubscriberMatched() const {
    return false;
}
bool FourierDdsTransport::isErrorCodesSubscriberMatched() const {
    return false;
}
void FourierDdsTransport::stop() {}

#endif

namespace detail {

#if defined(FOURIER_AURORA_SDK_TESTING)
namespace {

std::mutex test_factory_mutex;
TransportFactory test_factory;

}  // namespace

void setTransportFactoryForTesting(TransportFactory factory) {
    if (!factory) {
        throw InvalidArgumentError("test transport factory must not be empty");
    }
    std::lock_guard<std::mutex> lock(test_factory_mutex);
    test_factory = std::move(factory);
}

void resetTransportFactoryForTesting() {
    std::lock_guard<std::mutex> lock(test_factory_mutex);
    test_factory = nullptr;
}
#endif

class FourierDdsTransportAdapter final : public ITransport {
public:
    explicit FourierDdsTransportAdapter(
        std::unique_ptr<FourierDdsTransport> transport)
        : transport_(std::move(transport)) {}

    OperationResult publishMotionCommand(
        const MotionCommand& command) override {
        return transport_->publishMotionCommand(command);
    }

    OperationResult publishControlGroupCommand(
        const ControlGroupCommand& command) override {
        return transport_->publishControlGroupCommand(command);
    }

    OperationResult publishSubControllerCommand(
        const SubControllerCommand& command) override {
        return transport_->publishSubControllerCommand(command);
    }

    OperationResult requestFsmState(uint32_t state_id) override {
        return transport_->requestFsmState(state_id);
    }

    EndpointMatchStatus endpointMatchStatus() const override {
        return transport_->endpointMatchStatus();
    }

    void stop() override {
        transport_->stop();
    }

private:
    std::unique_ptr<FourierDdsTransport> transport_;
};

class FourierDdsTransportFactory {
public:
    static std::unique_ptr<ITransport> create(
        uint32_t domain_id,
        const ConnectionOptions& options,
        TransportHandlers handlers,
        TransportTerminalObserver terminal_observer) {
        FourierDdsTransport::InitialHandlers initial;
        initial.callbacks = std::move(handlers);
        initial.terminal_observer = std::move(terminal_observer);
        initial.callbacks_are_user = false;
        auto transport = std::unique_ptr<FourierDdsTransport>(
            new FourierDdsTransport(domain_id, options, std::move(initial)));
        return std::make_unique<FourierDdsTransportAdapter>(
            std::move(transport));
    }
};

std::unique_ptr<ITransport> makeProductionTransport(
    uint32_t domain_id,
    const ConnectionOptions& options,
    TransportHandlers handlers,
    TransportTerminalObserver terminal_observer) {
#if defined(FOURIER_AURORA_SDK_TESTING)
    TransportFactory factory;
    {
        std::lock_guard<std::mutex> lock(test_factory_mutex);
        factory = test_factory;
    }
    if (factory) {
        return factory(
            domain_id,
            options,
            std::move(handlers),
            std::move(terminal_observer));
    }
#endif
    return FourierDdsTransportFactory::create(
        domain_id,
        options,
        std::move(handlers),
        std::move(terminal_observer));
}

}  // namespace detail

}  // namespace fourier::aurora_sdk
