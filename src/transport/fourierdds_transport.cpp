#include "fourier_aurora_sdk/transport/fourierdds_transport.hpp"

#include <algorithm>
#include <chrono>
#include <iostream>
#include <memory>
#include <mutex>
#include <set>
#include <stdexcept>
#include <string>
#include <utility>
#include <vector>

#include "fourier_aurora_sdk/errors.hpp"

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

namespace fourier::aurora_sdk {

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

void logConversionErrorOnce(const std::string& topic, const std::exception& ex) {
    static std::mutex mutex;
    static std::set<std::string> seen;
    const std::string message = "Failed to convert " + topic + ": " + ex.what();
    std::lock_guard<std::mutex> lock(mutex);
    if (seen.insert(message).second) {
        std::cerr << message << std::endl;
    }
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

    std::vector<dds_msg::CartesianCmd> cartesian_cmds;
    cartesian_cmds.reserve(command.cartesian_cmd.size());
    for (const auto& command_entry : command.cartesian_cmd) {
        dds_msg::CartesianCmd entry;
        entry.link_name(command_entry.link_name);
        entry.pose(toVector(command_entry.pose));
        entry.twist(toVector(command_entry.twist));
        entry.wrench(toVector(command_entry.wrench));
        cartesian_cmds.push_back(std::move(entry));
    }
    msg.cartesian_cmd(std::move(cartesian_cmds));
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

struct FourierDdsTransport::Impl {
    using Node = fourier_dds::DdsNode;

    explicit Impl(uint32_t domain_id, const NormalizedConnectionOptions& options)
        : node(std::make_shared<Node>(
              domain_id,
              options.dds_mode,
              options.has_ros_compatible_override ? std::optional<bool>(options.is_ros_compatible) : std::nullopt,
              options.use_namespace,
              options.has_discovery_server_override ? std::optional<bool>(options.use_discovery_server) : std::nullopt)) {
        motion_command_pub = std::make_unique<MotionCommandPublisher>(node, "robot_motion_cmd");
        control_group_command_pub =
            std::make_unique<ControlGroupCommandPublisher>(node, "robot_control_group_cmd");
        sub_controller_command_pub =
            std::make_unique<SubControllerCommandPublisher>(node, "sub_controller_cmd");

        aurora_state_sub = std::make_unique<AuroraStateSubscription>(
            node, "aurora_state", [this](const dds_msg::AuroraState& msg) {
                AuroraStateHandler handler;
                {
                    std::lock_guard<std::mutex> lock(mutex);
                    handler = aurora_state_handler;
                }
                if (handler) {
                    try {
                        handler(convertAuroraState(msg));
                    } catch (const std::exception& ex) {
                        logConversionErrorOnce("aurora_state", ex);
                    }
                }
            });
        motion_state_sub = std::make_unique<MotionStateSubscription>(
            node, "robot_motion_state", [this](const dds_msg::RobotMotionState& msg) {
                MotionStateHandler handler;
                {
                    std::lock_guard<std::mutex> lock(mutex);
                    handler = motion_state_handler;
                }
                if (handler) {
                    try {
                        handler(convertMotionState(msg));
                    } catch (const std::exception& ex) {
                        logConversionErrorOnce("robot_motion_state", ex);
                    }
                }
            });
        control_group_state_sub = std::make_unique<ControlGroupStateSubscription>(
            node, "robot_control_group_state", [this](const dds_msg::RobotControlGroupState& msg) {
                ControlGroupStateHandler handler;
                {
                    std::lock_guard<std::mutex> lock(mutex);
                    handler = control_group_state_handler;
                }
                if (handler) {
                    try {
                        handler(convertControlGroupState(msg));
                    } catch (const std::exception& ex) {
                        logConversionErrorOnce("robot_control_group_state", ex);
                    }
                }
            });
        estimator_data_sub = std::make_unique<EstimatorDataSubscription>(
            node, "estimator_data", [this](const dds_msg::EstimatorData& msg) {
                EstimatorDataHandler handler;
                {
                    std::lock_guard<std::mutex> lock(mutex);
                    handler = estimator_data_handler;
                }
                if (handler) {
                    try {
                        handler(convertEstimatorData(msg));
                    } catch (const std::exception& ex) {
                        logConversionErrorOnce("estimator_data", ex);
                    }
                }
            });
        error_codes_sub = std::make_unique<ErrorCodesSubscription>(
            node, "error_codes_pub", [this](const dds_msg::ErrorCodes& msg) {
                ErrorCodesHandler handler;
                {
                    std::lock_guard<std::mutex> lock(mutex);
                    handler = error_codes_handler;
                }
                if (handler) {
                    try {
                        handler(convertErrorCodes(msg));
                    } catch (const std::exception& ex) {
                        logConversionErrorOnce("error_codes_pub", ex);
                    }
                }
            });

        fsm_service = std::make_unique<FsmClient>(node, "whole_body_fsm_state_change_service");
    }

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

    mutable std::mutex mutex;
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
};

FourierDdsTransport::FourierDdsTransport(
    uint32_t domain_id,
    const NormalizedConnectionOptions& options)
    : impl_(std::make_unique<Impl>(domain_id, options)) {}

FourierDdsTransport::~FourierDdsTransport() {
    close();
}

void FourierDdsTransport::setAuroraStateHandler(AuroraStateHandler handler) {
    std::lock_guard<std::mutex> lock(impl_->mutex);
    impl_->aurora_state_handler = std::move(handler);
}

void FourierDdsTransport::setMotionStateHandler(MotionStateHandler handler) {
    std::lock_guard<std::mutex> lock(impl_->mutex);
    impl_->motion_state_handler = std::move(handler);
}

void FourierDdsTransport::setControlGroupStateHandler(ControlGroupStateHandler handler) {
    std::lock_guard<std::mutex> lock(impl_->mutex);
    impl_->control_group_state_handler = std::move(handler);
}

void FourierDdsTransport::setEstimatorDataHandler(EstimatorDataHandler handler) {
    std::lock_guard<std::mutex> lock(impl_->mutex);
    impl_->estimator_data_handler = std::move(handler);
}

void FourierDdsTransport::setErrorCodesHandler(ErrorCodesHandler handler) {
    std::lock_guard<std::mutex> lock(impl_->mutex);
    impl_->error_codes_handler = std::move(handler);
}

void FourierDdsTransport::publishMotionCommand(const MotionCommand& command) {
    if (!impl_->motion_command_pub->publish(convertMotionCommand(command))) {
        throw PublishError("failed to publish robot_motion_cmd");
    }
}

void FourierDdsTransport::publishControlGroupCommand(const ControlGroupCommand& command) {
    if (!impl_->control_group_command_pub->publish(convertControlGroupCommand(command))) {
        throw PublishError("failed to publish robot_control_group_cmd");
    }
}

void FourierDdsTransport::publishSubControllerCommand(const SubControllerCommand& command) {
    if (!impl_->sub_controller_command_pub->publish(convertSubControllerCommand(command))) {
        throw PublishError("failed to publish sub_controller_cmd");
    }
}

ServiceCallResult FourierDdsTransport::requestFsmState(uint32_t state_id, uint32_t timeout_ms) {
    ServiceCallResult result;
    result.request_send_time = std::chrono::system_clock::now();

    dds_srv::SetInt_Request request;
    request.header(makeTracerHeader(result.request_send_time));
    request.data(static_cast<uint64_t>(state_id));
    dds_srv::SetInt_Response response;
    const bool ok = impl_->fsm_service->send_request(request, response, timeout_ms);
    result.response_receive_time = std::chrono::system_clock::now();
    if (!ok) {
        result.timed_out = true;
        result.message = "whole_body_fsm_state_change_service request timed out or failed";
        return result;
    }
    result.result_code = response.result();
    result.message = response.message();
    return result;
}

bool FourierDdsTransport::isMotionCommandPublisherMatched() const {
    return impl_->motion_command_pub && impl_->motion_command_pub->get_matched() > 0;
}

bool FourierDdsTransport::isControlGroupCommandPublisherMatched() const {
    return impl_->control_group_command_pub && impl_->control_group_command_pub->get_matched() > 0;
}

bool FourierDdsTransport::isSubControllerCommandPublisherMatched() const {
    return impl_->sub_controller_command_pub && impl_->sub_controller_command_pub->get_matched() > 0;
}

bool FourierDdsTransport::isAuroraStateSubscriberMatched() const {
    return impl_->aurora_state_sub && impl_->aurora_state_sub->get_matched() > 0;
}

bool FourierDdsTransport::isMotionStateSubscriberMatched() const {
    return impl_->motion_state_sub && impl_->motion_state_sub->get_matched() > 0;
}

bool FourierDdsTransport::isControlGroupStateSubscriberMatched() const {
    return impl_->control_group_state_sub && impl_->control_group_state_sub->get_matched() > 0;
}

bool FourierDdsTransport::isEstimatorDataSubscriberMatched() const {
    return impl_->estimator_data_sub && impl_->estimator_data_sub->get_matched() > 0;
}

bool FourierDdsTransport::isErrorCodesSubscriberMatched() const {
    return impl_->error_codes_sub && impl_->error_codes_sub->get_matched() > 0;
}

void FourierDdsTransport::close() {
    if (!impl_) {
        return;
    }

    std::shared_ptr<Impl::Node> node;
    std::unique_ptr<Impl::FsmClient> fsm_service;
    std::unique_ptr<Impl::MotionCommandPublisher> motion_command_pub;
    std::unique_ptr<Impl::ControlGroupCommandPublisher> control_group_command_pub;
    std::unique_ptr<Impl::SubControllerCommandPublisher> sub_controller_command_pub;
    std::unique_ptr<Impl::AuroraStateSubscription> aurora_state_sub;
    std::unique_ptr<Impl::MotionStateSubscription> motion_state_sub;
    std::unique_ptr<Impl::ControlGroupStateSubscription> control_group_state_sub;
    std::unique_ptr<Impl::EstimatorDataSubscription> estimator_data_sub;
    std::unique_ptr<Impl::ErrorCodesSubscription> error_codes_sub;

    {
        std::lock_guard<std::mutex> lock(impl_->mutex);
        impl_->aurora_state_handler = nullptr;
        impl_->motion_state_handler = nullptr;
        impl_->control_group_state_handler = nullptr;
        impl_->estimator_data_handler = nullptr;
        impl_->error_codes_handler = nullptr;

        error_codes_sub = std::move(impl_->error_codes_sub);
        estimator_data_sub = std::move(impl_->estimator_data_sub);
        control_group_state_sub = std::move(impl_->control_group_state_sub);
        motion_state_sub = std::move(impl_->motion_state_sub);
        aurora_state_sub = std::move(impl_->aurora_state_sub);
        sub_controller_command_pub = std::move(impl_->sub_controller_command_pub);
        control_group_command_pub = std::move(impl_->control_group_command_pub);
        motion_command_pub = std::move(impl_->motion_command_pub);
        fsm_service = std::move(impl_->fsm_service);
        node = std::move(impl_->node);
    }
}

}  // namespace fourier::aurora_sdk
