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
#include <optional>
#include <stdexcept>
#include <string>
#include <type_traits>
#include <utility>
#include <vector>

#include "fourier_aurora_sdk/errors.hpp"
#include "internal/callback_context.hpp"
#include "internal/connection_options.hpp"
#include "internal/estimator_root_link.hpp"
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
#include "LeaseInfo/LeaseInfo.hpp"
#include "ManageLease/ManageLease.hpp"
#include "ManageLease/ManageLeasePubSubTypes.hpp"
#include "MotorCfgCmd/MotorCfgCmd.hpp"
#include "MotorCfgCmd/MotorCfgCmdPubSubTypes.hpp"
#include "MotorCfgState/MotorCfgState.hpp"
#include "MotorCfgState/MotorCfgStatePubSubTypes.hpp"
#include "RobotControlGroupCmd/RobotControlGroupCmd.hpp"
#include "RobotControlGroupCmd/RobotControlGroupCmdPubSubTypes.hpp"
#include "RobotControlGroupState/RobotControlGroupState.hpp"
#include "RobotControlGroupState/RobotControlGroupStatePubSubTypes.hpp"
#include "RobotMotionCmd/RobotMotionCmd.hpp"
#include "RobotMotionCmd/RobotMotionCmdPubSubTypes.hpp"
#include "RobotMotionState/RobotMotionState.hpp"
#include "RobotMotionState/RobotMotionStatePubSubTypes.hpp"
#include "FsmStateSwitch/FsmStateSwitch.hpp"
#include "FsmStateSwitch/FsmStateSwitchPubSubTypes.hpp"
#include "SubControllerCmd/SubControllerCmd.hpp"
#include "SubControllerCmd/SubControllerCmdPubSubTypes.hpp"
#include "SubControllerService/SubControllerService.hpp"
#include "SubControllerService/SubControllerServicePubSubTypes.hpp"
#endif

namespace fourier::aurora_sdk {

#if defined(FOURIER_AURORA_SDK_ENABLE_DDS_TRANSPORT)
namespace dds_msg = fourier_msgs::msg;
namespace dds_pod_msg = fourier_msgs_pod::msg;
namespace dds_srv = fourier_msgs::srv;

namespace {

constexpr std::array<const char *, 10> kResourceNames{"left_leg",
                                                      "right_leg",
                                                      "waist",
                                                      "head",
                                                      "left_manipulator",
                                                      "right_manipulator",
                                                      "left_end_effector",
                                                      "right_end_effector",
                                                      "motion_cmd",
                                                      "cartesian_cmd"};

void warnUnknownAuthorityResource(const std::string &resource_name) {
  static std::mutex mutex;
  static std::chrono::steady_clock::time_point last_log;
  static uint32_t suppressed = 0;
  static bool initialized = false;

  const auto now = std::chrono::steady_clock::now();
  std::lock_guard<std::mutex> lock(mutex);
  if (initialized && now - last_log < std::chrono::seconds(5)) {
    ++suppressed;
    return;
  }
  std::cerr << "[fourier_aurora_sdk] unknown authority resource '"
            << resource_name << "' ignored";
  if (suppressed != 0) {
    std::cerr << " (" << suppressed << " similar warnings suppressed)";
  }
  std::cerr << std::endl;
  initialized = true;
  last_log = now;
  suppressed = 0;
}

bool validSourceName(const std::string &name) {
  return !name.empty() && name.size() <= 64 &&
         std::all_of(name.begin(), name.end(), [](unsigned char c) {
           return (c >= 'A' && c <= 'Z') || (c >= 'a' && c <= 'z') ||
                  (c >= '0' && c <= '9') || c == '.' || c == '_' || c == '-';
         });
}

template <size_t N>
std::array<double, N> toArray(const std::vector<double> &values,
                              const char *field_name) {
  if (values.size() != N) {
    throw InvalidArgumentError(std::string(field_name) + " expected length " +
                               std::to_string(N) + ", got " +
                               std::to_string(values.size()));
  }
  std::array<double, N> out{};
  std::copy(values.begin(), values.end(), out.begin());
  return out;
}

template <size_t N>
std::array<double, N> toArrayOrDefault(const std::vector<double> &values,
                                       const char *field_name) {
  if (values.empty()) {
    return {};
  }
  return toArray<N>(values, field_name);
}

template <size_t N>
std::vector<double> toVector(const std::array<double, N> &values) {
  return std::vector<double>(values.begin(), values.end());
}

std::chrono::system_clock::time_point
toTimePoint(const builtin_interfaces::msg::Time &stamp) {
  const auto seconds = std::chrono::seconds(stamp.sec());
  const auto nanoseconds = std::chrono::nanoseconds(stamp.nanosec());
  return std::chrono::system_clock::time_point(seconds + nanoseconds);
}

builtin_interfaces::msg::Time
toDdsTime(std::chrono::system_clock::time_point time) {
  const auto since_epoch = time.time_since_epoch();
  const auto seconds =
      std::chrono::duration_cast<std::chrono::seconds>(since_epoch);
  const auto nanoseconds = std::chrono::duration_cast<std::chrono::nanoseconds>(
      since_epoch - seconds);
  builtin_interfaces::msg::Time stamp;
  stamp.sec(static_cast<int32_t>(seconds.count()));
  stamp.nanosec(static_cast<uint32_t>(nanoseconds.count()));
  return stamp;
}

MessageHeader
convertHeader(const dds_msg::TracerHeader &header,
              std::chrono::system_clock::time_point received_time) {
  MessageHeader out;
  out.send_time = toTimePoint(header.stamp());
  out.frame_id = header.frame_id();
  out.received_time = received_time;
  return out;
}

MessageHeader
convertHeader(const std_msgs::msg::Header &header,
              std::chrono::system_clock::time_point received_time) {
  MessageHeader out;
  out.send_time = toTimePoint(header.stamp());
  out.frame_id = header.frame_id();
  out.received_time = received_time;
  return out;
}

dds_msg::TracerHeader
makeTracerHeader(std::chrono::system_clock::time_point send_time) {
  dds_msg::TracerHeader header;
  header.stamp(toDdsTime(send_time));
  header.frame_id("");
  return header;
}

StateInfo convertStateInfo(const dds_msg::StateInfo &state) {
  StateInfo out;
  out.id = state.id();
  out.version = state.version();
  out.name = state.name();
  out.description = state.description();
  out.active = state.active();
  return out;
}

AuroraState convertAuroraState(const dds_msg::AuroraState &msg) {
  const auto received_time = std::chrono::system_clock::now();
  AuroraState out;
  out.header = convertHeader(msg.header(), received_time);
  out.robot_info.robot_type = msg.robot_info().robot_type();
  out.robot_info.hardware_type = msg.robot_info().hardware_type();
  out.robot_info.end_effector_type = msg.robot_info().end_effector_type();
  out.switch_status = msg.switch_status();
  out.current_state = convertStateInfo(msg.current_state());
  out.available_states.reserve(msg.available_states().size());
  for (const auto &state : msg.available_states()) {
    out.available_states.push_back(convertStateInfo(state));
  }
  std::array<std::optional<AuthorityResourceStatus>, kResourceNames.size()>
      decoded_resources;
  for (const auto &resource : msg.authority_resources()) {
    const std::string resource_name{resource.resource().c_str()};
    if (resource_name.empty()) {
      throw std::runtime_error("authority resource name is empty");
    }
    const auto name_it =
        std::find(kResourceNames.begin(), kResourceNames.end(), resource_name);
    if (name_it == kResourceNames.end()) {
      warnUnknownAuthorityResource(resource_name);
      continue;
    }
    const auto index = static_cast<std::size_t>(
        std::distance(kResourceNames.begin(), name_it));
    if (decoded_resources[index].has_value()) {
      throw std::runtime_error("duplicate authority resource '" +
                               resource_name + "'");
    }
    const std::string holder_name = resource.holder_source_name().to_string();
    if (resource.holder_priority() != 0 && resource.holder_priority() != 9) {
      throw std::runtime_error("authority resource '" + resource_name +
                               "' has invalid holder priority");
    }
    if (resource.acquired() && !validSourceName(holder_name)) {
      throw std::runtime_error("acquired authority resource '" + resource_name +
                               "' has an invalid holder source name");
    }
    if (!resource.acquired() &&
        (!holder_name.empty() || resource.holder_priority() != 0)) {
      throw std::runtime_error("unacquired authority resource '" +
                               resource_name + "' contains holder state");
    }
    decoded_resources[index] = AuthorityResourceStatus{
        static_cast<AuroraResource>(index), resource.acquired(), holder_name,
        static_cast<LeasePriority>(resource.holder_priority())};
  }
  out.authority_resources.reserve(kResourceNames.size());
  for (auto &resource : decoded_resources) {
    if (resource.has_value()) {
      out.authority_resources.push_back(std::move(*resource));
    }
  }
  return out;
}

MotionState convertMotionState(const dds_msg::RobotMotionState &msg) {
  const auto received_time = std::chrono::system_clock::now();
  MotionState out;
  out.header = convertHeader(msg.header(), received_time);
  out.position = toArray<3>(msg.position(), "RobotMotionState.position");
  out.position_cmd =
      toArray<3>(msg.position_cmd(), "RobotMotionState.position_cmd");
  out.lin_velocity =
      toArray<3>(msg.lin_velocity(), "RobotMotionState.lin_velocity");
  out.lin_velocity_cmd =
      toArray<3>(msg.lin_velocity_cmd(), "RobotMotionState.lin_velocity_cmd");
  out.attitude = toArray<3>(msg.attitude(), "RobotMotionState.attitude");
  out.attitude_cmd =
      toArray<3>(msg.attitude_cmd(), "RobotMotionState.attitude_cmd");
  out.ang_velocity =
      toArray<3>(msg.ang_velocity(), "RobotMotionState.ang_velocity");
  out.ang_velocity_cmd =
      toArray<3>(msg.ang_velocity_cmd(), "RobotMotionState.ang_velocity_cmd");
  out.delta_height = msg.delta_height();
  out.delta_height_cmd = msg.delta_height_cmd();
  out.progress = msg.progress();
  out.stable_level = msg.stable_level();
  return out;
}

ControlGroupStateEntry
convertControlGroupEntry(const dds_msg::ControlGroupState &entry) {
  ControlGroupStateEntry out;
  out.group_name = entry.group_name();
  out.joint_position = entry.joint_position();
  out.joint_velocity = entry.joint_velocity();
  out.joint_effort = entry.joint_effort();
  return out;
}

CartesianStateEntry
convertCartesianEntry(const dds_msg::CartesianState &entry) {
  CartesianStateEntry out;
  out.link_name = entry.link_name();
  out.pose = toArrayOrDefault<7>(entry.pose(), "CartesianState.pose");
  out.twist = toArrayOrDefault<6>(entry.twist(), "CartesianState.twist");
  out.wrench = toArrayOrDefault<6>(entry.wrench(), "CartesianState.wrench");
  return out;
}

CartesianCommand
convertCartesianCommandEntry(const dds_msg::CartesianState &entry) {
  CartesianCommand out;
  out.link_name = entry.link_name();
  out.pose = entry.pose();
  out.twist = entry.twist();
  out.wrench = entry.wrench();
  return out;
}

ControlGroupState
convertControlGroupState(const dds_msg::RobotControlGroupState &msg) {
  const auto received_time = std::chrono::system_clock::now();
  ControlGroupState out;
  out.header = convertHeader(msg.header(), received_time);
  out.joint_state.reserve(msg.joint_state().size());
  for (const auto &entry : msg.joint_state()) {
    out.joint_state.push_back(convertControlGroupEntry(entry));
  }
  out.joint_cmd.reserve(msg.joint_cmd().size());
  for (const auto &entry : msg.joint_cmd()) {
    out.joint_cmd.push_back(convertControlGroupEntry(entry));
  }
  out.cartesian_state.reserve(msg.cartesian_state().size());
  for (const auto &entry : msg.cartesian_state()) {
    out.cartesian_state.push_back(convertCartesianEntry(entry));
  }
  out.cartesian_cmd.reserve(msg.cartesian_cmd().size());
  for (const auto &entry : msg.cartesian_cmd()) {
    out.cartesian_cmd.push_back(convertCartesianCommandEntry(entry));
  }
  return out;
}

EstimatorData convertEstimatorData(const dds_msg::EstimatorData &msg) {
  const auto received_time = std::chrono::system_clock::now();
  EstimatorData out;
  out.header = convertHeader(msg.header(), received_time);
  out.root_link = detail::estimatorRootLink(msg, 0);
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

ErrorCodes convertErrorCodes(const dds_msg::ErrorCodes &msg) {
  const auto received_time = std::chrono::system_clock::now();
  ErrorCodes out;
  out.header = convertHeader(msg.header(), received_time);
  out.source = msg.source();
  out.error_codes.reserve(msg.error_codes().size());
  for (const auto &code : msg.error_codes()) {
    out.error_codes.push_back(ErrorCodeEntry{code.high32(), code.low32()});
  }
  return out;
}

MotorConfigurationState
convertMotorConfigurationState(const dds_msg::RobotMotorCfgGroupState &msg) {
  MotorConfigurationState out;
  out.header = convertHeader(msg.header(), std::chrono::system_clock::now());
  out.groups.reserve(msg.group_state().size());
  for (const auto &group : msg.group_state()) {
    MotorConfigurationGroupState value;
    value.group_name = group.group_name();
    value.mode = static_cast<MotorConfigurationMode>(group.mode());
    value.motor_error = group.motor_error();
    value.position_kp = group.pos_kp();
    value.position_ki = group.pos_ki();
    value.position_kd = group.pos_kd();
    value.pd_kp = group.pd_kp();
    value.pd_kd = group.pd_kd();
    out.groups.push_back(std::move(value));
  }
  return out;
}

dds_pod_msg::LeaseInfo convertLease(const detail::LeaseToken &lease) {
  dds_pod_msg::LeaseInfo out;
  out.lease_id(lease.lease_id);
  out.sequence_id(lease.sequence_id);
  return out;
}

dds_msg::RobotMotionCmd convertMotionCommand(const MotionCommand &command,
                                             const detail::LeaseToken &lease) {
  dds_msg::RobotMotionCmd msg;
  msg.header(makeTracerHeader(command.header.send_time));
  msg.lease(convertLease(lease));
  msg.position(toVector(command.position));
  msg.lin_velocity(toVector(command.lin_velocity));
  msg.attitude(toVector(command.attitude));
  msg.ang_velocity(toVector(command.ang_velocity));
  msg.delta_height(command.delta_height);
  msg.gait_type(command.gait_type);
  msg.skills(
      std::vector<uint8_t>(command.skills.begin(), command.skills.end()));
  msg.duration(command.duration);
  return msg;
}

dds_msg::RobotControlGroupCmd
convertControlGroupCommand(const ControlGroupCommand &command,
                           const detail::LeaseToken &lease) {
  dds_msg::RobotControlGroupCmd msg;
  msg.header(makeTracerHeader(command.header.send_time));
  msg.lease(convertLease(lease));
  std::vector<dds_msg::JointCmd> joint_cmds;
  joint_cmds.reserve(command.joint_cmd.size());
  for (const auto &command_entry : command.joint_cmd) {
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
  for (const auto &command_entry : command.cartesian_cmd) {
    dds_msg::CartesianCmd entry;
    entry.link_name(command_entry.link_name);
    entry.pose(command_entry.pose);
    entry.twist(command_entry.twist);
    entry.wrench(command_entry.wrench);
    cartesian_cmds.push_back(std::move(entry));
  }
  msg.cartesian_cmd(std::move(cartesian_cmds));
  return msg;
}

dds_msg::RobotMotorCfgGroupCmd
convertMotorConfigurationCommand(const MotorConfigurationCommand &command,
                                 const detail::LeaseToken &lease) {
  dds_msg::RobotMotorCfgGroupCmd msg;
  std_msgs::msg::Header header;
  header.stamp(toDdsTime(command.header.send_time));
  header.frame_id("");
  msg.header(std::move(header));
  msg.lease(convertLease(lease));
  std::vector<dds_msg::MotorCfgGroupCmd> groups;
  groups.reserve(command.groups.size());
  for (const auto &group : command.groups) {
    dds_msg::MotorCfgGroupCmd value;
    value.group_name(group.group_name);
    value.mode(static_cast<dds_msg::MotorMode>(group.mode));
    value.pos_kp(group.position_kp);
    value.pos_ki(group.position_ki);
    value.pos_kd(group.position_kd);
    value.pd_kp(group.pd_kp);
    value.pd_kd(group.pd_kd);
    groups.push_back(std::move(value));
  }
  msg.group_cmd(std::move(groups));
  return msg;
}

dds_msg::SubControllerCmd
convertSubControllerCommand(const SubControllerCommand &command) {
  dds_msg::SubControllerCmd msg;
  msg.header(makeTracerHeader(command.header.send_time));
  msg.reset(command.reset);

  std::vector<dds_msg::ControllerParam> params;
  params.reserve(command.controller_params.size());
  for (const auto &controller : command.controller_params) {
    dds_msg::ControllerParam controller_msg;
    controller_msg.controller_name(controller.controller_name);
    controller_msg.rate(controller.rate);
    std::vector<dds_msg::GroupOverrideParam> overrides;
    overrides.reserve(controller.group_override_params.size());
    for (const auto &override_param : controller.group_override_params) {
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

} // namespace

#endif

struct FourierDdsTransport::InitialHandlers {
  detail::TransportHandlers callbacks;
  detail::TransportTerminalObserver terminal_observer;
  bool callbacks_are_user = true;
};

#if defined(FOURIER_AURORA_SDK_ENABLE_DDS_TRANSPORT)

namespace {

OperationResult makeResult(uint32_t code, const std::string &detail = {}) {
  OperationResult result;
  result.code = code;
  result.message = std::string(defaultResultMessage(code));
  if (!detail.empty()) {
    result.message += ": ";
    result.message += detail;
  }
  return result;
}

const char *endpointName(Endpoint endpoint) {
  switch (endpoint) {
  case Endpoint::FsmService:
    return "fsm_service";
  case Endpoint::ManageLeaseService:
    return "manage_lease_service";
  case Endpoint::MotionCommandPublisher:
    return "motion_command_publisher";
  case Endpoint::ControlGroupCommandPublisher:
    return "control_group_command_publisher";
  case Endpoint::MotorConfigurationCommandPublisher:
    return "motor_configuration_command_publisher";
  case Endpoint::SubControllerService:
    return "sub_controller_service";
  case Endpoint::AuroraStateSubscriber:
    return "aurora_state_subscriber";
  case Endpoint::MotionStateSubscriber:
    return "motion_state_subscriber";
  case Endpoint::ControlGroupStateSubscriber:
    return "control_group_state_subscriber";
  case Endpoint::MotorConfigurationStateSubscriber:
    return "motor_configuration_state_subscriber";
  case Endpoint::EstimatorDataSubscriber:
    return "estimator_data_subscriber";
  case Endpoint::ErrorCodesSubscriber:
    return "error_codes_subscriber";
  }
  return "invalid_endpoint";
}

EndpointStatus &endpointStatus(EndpointMatchStatus &status, Endpoint endpoint) {
  switch (endpoint) {
  case Endpoint::FsmService:
    return status.fsm_service;
  case Endpoint::ManageLeaseService:
    return status.manage_lease_service;
  case Endpoint::MotionCommandPublisher:
    return status.motion_command_publisher;
  case Endpoint::ControlGroupCommandPublisher:
    return status.control_group_command_publisher;
  case Endpoint::MotorConfigurationCommandPublisher:
    return status.motor_configuration_command_publisher;
  case Endpoint::SubControllerService:
    return status.sub_controller_service;
  case Endpoint::AuroraStateSubscriber:
    return status.aurora_state_subscriber;
  case Endpoint::MotionStateSubscriber:
    return status.motion_state_subscriber;
  case Endpoint::ControlGroupStateSubscriber:
    return status.control_group_state_subscriber;
  case Endpoint::MotorConfigurationStateSubscriber:
    return status.motor_configuration_state_subscriber;
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

} // namespace

struct FourierDdsTransport::Impl
    : public std::enable_shared_from_this<FourierDdsTransport::Impl> {
  using Node = fourier_dds::DdsNode;
  using MotionCommandPublisher =
      fourier_dds::DdsPublisher<dds_msg::RobotMotionCmdPubSubType>;
  using ControlGroupCommandPublisher =
      fourier_dds::DdsPublisher<dds_msg::RobotControlGroupCmdPubSubType>;
  using MotorConfigurationCommandPublisher =
      fourier_dds::DdsPublisher<dds_msg::RobotMotorCfgGroupCmdPubSubType>;
  using AuroraStateSubscription =
      fourier_dds::DdsSubscription<dds_msg::AuroraStatePubSubType>;
  using MotionStateSubscription =
      fourier_dds::DdsSubscription<dds_msg::RobotMotionStatePubSubType>;
  using ControlGroupStateSubscription =
      fourier_dds::DdsSubscription<dds_msg::RobotControlGroupStatePubSubType>;
  using MotorConfigurationStateSubscription =
      fourier_dds::DdsSubscription<dds_msg::RobotMotorCfgGroupStatePubSubType>;
  using EstimatorDataSubscription =
      fourier_dds::DdsSubscription<dds_msg::EstimatorDataPubSubType>;
  using ErrorCodesSubscription =
      fourier_dds::DdsSubscription<dds_msg::ErrorCodesPubSubType>;
  using FsmClient =
      fourier_dds::DdsClient<dds_srv::FsmStateSwitch_RequestPubSubType,
                             dds_srv::FsmStateSwitch_ResponsePubSubType>;
  using ManageLeaseClient =
      fourier_dds::DdsClient<dds_srv::ManageLease_RequestPubSubType,
                             dds_srv::ManageLease_ResponsePubSubType>;
  using SubControllerClient =
      fourier_dds::DdsClient<dds_srv::SubControllerService_RequestPubSubType,
                             dds_srv::SubControllerService_ResponsePubSubType>;

  class OperationAdmission {
  public:
    OperationAdmission() = default;

    explicit OperationAdmission(std::shared_ptr<Impl> session)
        : session_(std::move(session)) {}

    OperationAdmission(OperationAdmission &&other) noexcept
        : session_(std::move(other.session_)) {}

    OperationAdmission &operator=(OperationAdmission &&other) noexcept {
      if (this != &other) {
        release();
        session_ = std::move(other.session_);
      }
      return *this;
    }

    ~OperationAdmission() { release(); }

    OperationAdmission(const OperationAdmission &) = delete;
    OperationAdmission &operator=(const OperationAdmission &) = delete;

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

    CallbackAdmission(const CallbackAdmission &) = delete;
    CallbackAdmission &operator=(const CallbackAdmission &) = delete;

  private:
    std::shared_ptr<Impl> session_;
  };

  struct AsyncLogSlot {
    std::chrono::steady_clock::time_point last_log{};
    uint32_t suppressed = 0;
    bool initialized = false;
  };

  static std::shared_ptr<Impl> create(const ConnectionOptions &public_options,
                                      InitialHandlers initial_handlers) {
    auto &coordinator = detail::TeardownCoordinator::instance();
    coordinator.ensureWorker();

    auto session = std::shared_ptr<Impl>(
        new Impl(detail::normalizeConnectionOptions(public_options),
                 std::move(initial_handlers)));
    try {
      session->initialize(session->options.public_options.domain_id);
    } catch (const std::exception &error) {
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

  Impl(detail::NormalizedConnectionOptions normalized_options,
       InitialHandlers initial_handlers)
      : options(std::move(normalized_options)),
        aurora_state_handler(
            std::move(initial_handlers.callbacks.aurora_state)),
        motion_state_handler(
            std::move(initial_handlers.callbacks.motion_state)),
        control_group_state_handler(
            std::move(initial_handlers.callbacks.control_group_state)),
        motor_configuration_state_handler(
            std::move(initial_handlers.callbacks.motor_configuration_state)),
        estimator_data_handler(
            std::move(initial_handlers.callbacks.estimator_data)),
        error_codes_handler(std::move(initial_handlers.callbacks.error_codes)),
        terminal_observer(std::move(initial_handlers.terminal_observer)),
        callbacks_are_user(initial_handlers.callbacks_are_user) {}

  void initialize(uint32_t domain_id) {
    const auto &public_options = options.public_options;
    const bool node_uses_namespace =
        options.has_namespace_override ? false : public_options.use_namespace;
    node = std::make_shared<Node>(
        domain_id, public_options.dds_mode,
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

    const auto endpoint_name = [&public_options](const char *base_name) {
      return public_options.topic_prefix + base_name;
    };

    if (options.endpointEnabled(Endpoint::MotionCommandPublisher)) {
      motion_command_pub = std::make_unique<MotionCommandPublisher>(
          node, endpoint_name("robot_motion_cmd"));
    }
    if (options.endpointEnabled(Endpoint::ControlGroupCommandPublisher)) {
      control_group_command_pub =
          std::make_unique<ControlGroupCommandPublisher>(
              node, endpoint_name("robot_control_group_cmd"));
    }
    if (options.endpointEnabled(Endpoint::MotorConfigurationCommandPublisher)) {
      motor_configuration_command_pub =
          std::make_unique<MotorConfigurationCommandPublisher>(
              node, endpoint_name("robot_motor_cfg_group_cmd"));
    }

    const std::weak_ptr<Impl> weak = shared_from_this();
    if (options.endpointEnabled(Endpoint::AuroraStateSubscriber)) {
      aurora_state_sub = std::make_unique<AuroraStateSubscription>(
          node, endpoint_name("aurora_state"),
          [weak](const dds_msg::AuroraState &message) {
            if (const auto session = weak.lock()) {
              session->dispatch(
                  Endpoint::AuroraStateSubscriber, &Impl::aurora_state_handler,
                  [&message] { return convertAuroraState(message); });
            }
          },
          false, fourier_dds::SubscriptionDurability::Volatile);
    }
    if (options.endpointEnabled(Endpoint::MotionStateSubscriber)) {
      motion_state_sub = std::make_unique<MotionStateSubscription>(
          node, endpoint_name("robot_motion_state"),
          [weak](const dds_msg::RobotMotionState &message) {
            if (const auto session = weak.lock()) {
              session->dispatch(
                  Endpoint::MotionStateSubscriber, &Impl::motion_state_handler,
                  [&message] { return convertMotionState(message); });
            }
          },
          false, fourier_dds::SubscriptionDurability::Volatile);
    }
    if (options.endpointEnabled(Endpoint::ControlGroupStateSubscriber)) {
      control_group_state_sub = std::make_unique<ControlGroupStateSubscription>(
          node, endpoint_name("robot_control_group_state"),
          [weak](const dds_msg::RobotControlGroupState &message) {
            if (const auto session = weak.lock()) {
              session->dispatch(Endpoint::ControlGroupStateSubscriber,
                                &Impl::control_group_state_handler, [&message] {
                                  return convertControlGroupState(message);
                                });
            }
          },
          false, fourier_dds::SubscriptionDurability::Volatile);
    }
    if (options.endpointEnabled(Endpoint::MotorConfigurationStateSubscriber)) {
      motor_configuration_state_sub =
          std::make_unique<MotorConfigurationStateSubscription>(
              node, endpoint_name("robot_motor_cfg_group_state"),
              [weak](const dds_msg::RobotMotorCfgGroupState &message) {
                if (const auto session = weak.lock()) {
                  session->dispatch(
                      Endpoint::MotorConfigurationStateSubscriber,
                      &Impl::motor_configuration_state_handler, [&message] {
                        return convertMotorConfigurationState(message);
                      });
                }
              },
              false, fourier_dds::SubscriptionDurability::Volatile);
    }
    if (options.endpointEnabled(Endpoint::EstimatorDataSubscriber)) {
      estimator_data_sub = std::make_unique<EstimatorDataSubscription>(
          node, endpoint_name("estimator_data"),
          [weak](const dds_msg::EstimatorData &message) {
            if (const auto session = weak.lock()) {
              session->dispatch(Endpoint::EstimatorDataSubscriber,
                                &Impl::estimator_data_handler, [&message] {
                                  return convertEstimatorData(message);
                                });
            }
          },
          false, fourier_dds::SubscriptionDurability::Volatile);
    }
    if (options.endpointEnabled(Endpoint::ErrorCodesSubscriber)) {
      error_codes_sub = std::make_unique<ErrorCodesSubscription>(
          node, endpoint_name("error_codes_pub"),
          [weak](const dds_msg::ErrorCodes &message) {
            if (const auto session = weak.lock()) {
              session->dispatch(
                  Endpoint::ErrorCodesSubscriber, &Impl::error_codes_handler,
                  [&message] { return convertErrorCodes(message); });
            }
          },
          false, fourier_dds::SubscriptionDurability::Volatile);
    }
    if (options.endpointEnabled(Endpoint::FsmService)) {
      fsm_service = std::make_unique<FsmClient>(
          node, endpoint_name("whole_body_fsm_state_change_service"));
    }
    if (options.endpointEnabled(Endpoint::ManageLeaseService)) {
      fourier_dds::ClientOptions client_options;
      client_options.log_requests = false;
      manage_lease_service = std::make_unique<ManageLeaseClient>(
          node, endpoint_name("ManageLease"), client_options);
    }
    if (options.endpointEnabled(Endpoint::SubControllerService)) {
      sub_controller_service = std::make_unique<SubControllerClient>(
          node, endpoint_name("sub_controller_cmd"));
    }
  }

  template <typename Handler, typename Converter>
  void dispatch(Endpoint endpoint, Handler Impl::*handler_member,
                Converter &&converter) {
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
    } catch (const std::exception &error) {
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
    } catch (const std::exception &error) {
      logAsyncFailure(endpoint, 1, error.what());
    } catch (...) {
      logAsyncFailure(endpoint, 1, "non-standard callback exception");
    }
  }

  void logAsyncFailure(Endpoint endpoint, std::size_t category,
                       const std::string &message) {
    const auto index = detail::endpointIndex(endpoint) * 2 + category;
    const auto now = std::chrono::steady_clock::now();
    uint32_t suppressed = 0;
    bool should_log = false;
    {
      std::lock_guard<std::mutex> lock(log_mutex);
      auto &slot = async_log_slots[index];
      if (!slot.initialized || now - slot.last_log >= std::chrono::seconds(5)) {
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
                << " failure on " << endpointName(endpoint) << ": " << message;
      if (suppressed != 0) {
        std::cerr << " (" << suppressed << " similar failures suppressed)";
      }
      std::cerr << std::endl;
    }
  }

  OperationAdmission beginOperation(Endpoint endpoint,
                                    OperationResult &failure) {
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

  [[noreturn]] void fatalInvariant(OperationAdmission &admission,
                                   const std::string &message) {
    std::cerr << "[fourier_aurora_sdk] fatal transport failure: " << message
              << std::endl;
    admission.release();
    auto completion = requestStop();
    if (!detail::inUserCallback()) {
      try {
        completion.get();
      } catch (const std::exception &error) {
        throw ConnectionError(message + "; teardown failed: " + error.what());
      }
    }
    throw ConnectionError(message);
  }

  template <typename Publisher, typename Converter>
  OperationResult publish(Endpoint endpoint, Publisher *publisher,
                          Converter &&converter) {
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
    } catch (const std::exception &error) {
      return makeResult(result_codes::CLIENT_ERROR, error.what());
    } catch (...) {
      return makeResult(result_codes::CLIENT_ERROR,
                        "non-standard command conversion failure");
    }

    try {
      if (!publisher->publish(message)) {
        return makeResult(result_codes::PUBLISH_FAILED);
      }
      return makeResult(result_codes::SUCCESS);
    } catch (const std::exception &error) {
      return makeResult(result_codes::TRANSPORT_ERROR, error.what());
    } catch (...) {
      return makeResult(result_codes::TRANSPORT_ERROR,
                        "non-standard DDS publish failure");
    }
  }

  OperationResult requestFsmState(uint32_t state_id, bool force_switch) {
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

    dds_srv::FsmStateSwitch_Request request;
    request.header(makeTracerHeader(std::chrono::system_clock::now()));
    request.source_name(options.public_options.source_name);
    request.target_state_id(state_id);
    request.force_switch(force_switch);
    dds_srv::FsmStateSwitch_Response response;
    const auto start = std::chrono::steady_clock::now();
    try {
      const bool received = fsm_service->send_request(
          request, response, options.public_options.service_timeout_ms);
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
    } catch (const std::exception &error) {
      if (!matched(Endpoint::FsmService)) {
        return makeResult(result_codes::ENDPOINT_UNMATCHED);
      }
      return makeResult(result_codes::TRANSPORT_ERROR, error.what());
    } catch (...) {
      return makeResult(result_codes::TRANSPORT_ERROR,
                        "non-standard DDS service failure");
    }

    uint32_t result_code = result_codes::INVALID_AURORA_RESPONSE;
    switch (response.result()) {
    case dds_srv::FSM_SWITCH_RESULT_ACCEPTED:
    case dds_srv::FSM_SWITCH_RESULT_ALREADY_ACTIVE:
      result_code = result_codes::SUCCESS;
      break;
    case dds_srv::FSM_SWITCH_RESULT_INVALID_REQUEST:
      result_code = result_codes::FSM_INVALID_REQUEST;
      break;
    case dds_srv::FSM_SWITCH_RESULT_STATE_NOT_FOUND:
      result_code = result_codes::FSM_STATE_NOT_FOUND;
      break;
    case dds_srv::FSM_SWITCH_RESULT_ENTER_CHECK_FAILED:
      result_code = result_codes::FSM_ENTER_CHECK_FAILED;
      break;
    case dds_srv::FSM_SWITCH_RESULT_SWITCH_BLOCKER_UNHEALTHY:
      result_code = result_codes::FSM_SWITCH_BLOCKER_UNHEALTHY;
      break;
    case dds_srv::FSM_SWITCH_RESULT_BUSY:
      result_code = result_codes::FSM_SWITCH_BUSY;
      break;
    case dds_srv::FSM_SWITCH_RESULT_SHUTTING_DOWN:
      result_code = result_codes::FSM_SHUTTING_DOWN;
      break;
    case dds_srv::FSM_SWITCH_RESULT_STATE_MANAGER_UNAVAILABLE:
      result_code = result_codes::FSM_STATE_MANAGER_UNAVAILABLE;
      break;
    case dds_srv::FSM_SWITCH_RESULT_INTERNAL_ERROR:
      result_code = result_codes::FSM_INTERNAL_ERROR;
      break;
    default:
      return makeResult(result_codes::INVALID_AURORA_RESPONSE);
    }
    return OperationResult{
        result_code,
        response.message().to_string(),
    };
  }

  detail::LeaseManagementResponse
  manageLease(const detail::LeaseManagementRequest &value,
              uint32_t timeout_ms) {
    detail::LeaseManagementResponse out;
    OperationResult failure;
    auto admission = beginOperation(Endpoint::ManageLeaseService, failure);
    if (!admission) {
      out.operation = failure;
      return out;
    }
    if (!manage_lease_service || !matched(Endpoint::ManageLeaseService)) {
      out.operation = makeResult(result_codes::ENDPOINT_UNMATCHED);
      return out;
    }
    dds_srv::ManageLease_Request request;
    request.header(makeTracerHeader(std::chrono::system_clock::now()));
    request.source_name(value.source_name);
    request.client_instance_id(value.client_instance_id);
    request.action(value.action);
    request.operation_id(value.operation_id);
    request.lease_id(value.lease_id);
    std::vector<dds_srv::LeaseResourceName> resources;
    resources.reserve(value.resources.size());
    for (const auto &name : value.resources) {
      dds_srv::LeaseResourceName resource;
      resource.name(name);
      resources.push_back(std::move(resource));
    }
    request.requested_resources(std::move(resources));
    request.priority(value.priority);
    request.renew_duration(value.renew_duration);

    dds_srv::ManageLease_Response response;
    try {
      if (!manage_lease_service->send_request(request, response, timeout_ms)) {
        out.operation = matched(Endpoint::ManageLeaseService)
                            ? makeResult(result_codes::SERVICE_TIMEOUT)
                            : makeResult(result_codes::ENDPOINT_UNMATCHED);
        return out;
      }
    } catch (const std::exception &error) {
      out.operation = makeResult(result_codes::TRANSPORT_ERROR, error.what());
      return out;
    } catch (...) {
      out.operation = makeResult(result_codes::TRANSPORT_ERROR);
      return out;
    }
    out.operation = makeResult(result_codes::SUCCESS);
    out.wire_result = response.result();
    out.lease_id = response.lease_id();
    out.renew_duration = response.renew_duration();
    out.message = response.message().to_string();
    return out;
  }

  OperationResult setSubController(const SubControllerCommand &command) {
    OperationResult failure;
    auto admission = beginOperation(Endpoint::SubControllerService, failure);
    if (!admission) {
      return failure;
    }
    if (!sub_controller_service || !matched(Endpoint::SubControllerService)) {
      return makeResult(result_codes::ENDPOINT_UNMATCHED);
    }
    dds_srv::SubControllerService_Request request;
    request.command(convertSubControllerCommand(command));
    dds_srv::SubControllerService_Response response;
    try {
      if (!sub_controller_service->send_request(
              request, response, options.public_options.service_timeout_ms)) {
        return matched(Endpoint::SubControllerService)
                   ? makeResult(result_codes::SERVICE_TIMEOUT)
                   : makeResult(result_codes::ENDPOINT_UNMATCHED);
      }
    } catch (const std::exception &error) {
      return makeResult(result_codes::TRANSPORT_ERROR, error.what());
    } catch (...) {
      return makeResult(result_codes::TRANSPORT_ERROR);
    }
    switch (response.result()) {
    case 0:
      return makeResult(result_codes::SUCCESS);
    case 1:
      return makeResult(result_codes::SUB_CONTROLLER_NO_ACTIVE_UNICONTROLLER,
                        response.message().to_string());
    case 2:
      return makeResult(result_codes::SUB_CONTROLLER_STATE_SWITCH_BUSY,
                        response.message().to_string());
    case 3:
      return makeResult(result_codes::SUB_CONTROLLER_INVALID_REQUEST,
                        response.message().to_string());
    case 4:
      return makeResult(result_codes::SUB_CONTROLLER_INTERNAL_ERROR,
                        response.message().to_string());
    default:
      return makeResult(result_codes::INVALID_AURORA_RESPONSE);
    }
  }

  bool matched(Endpoint endpoint) const {
    switch (endpoint) {
    case Endpoint::FsmService:
      return fsm_service && fsm_service->is_server_matched();
    case Endpoint::ManageLeaseService:
      return manage_lease_service && manage_lease_service->is_server_matched();
    case Endpoint::MotionCommandPublisher:
      return motion_command_pub && motion_command_pub->get_matched() > 0;
    case Endpoint::ControlGroupCommandPublisher:
      return control_group_command_pub &&
             control_group_command_pub->get_matched() > 0;
    case Endpoint::MotorConfigurationCommandPublisher:
      return motor_configuration_command_pub &&
             motor_configuration_command_pub->get_matched() > 0;
    case Endpoint::SubControllerService:
      return sub_controller_service &&
             sub_controller_service->is_server_matched();
    case Endpoint::AuroraStateSubscriber:
      return aurora_state_sub && aurora_state_sub->get_matched() > 0;
    case Endpoint::MotionStateSubscriber:
      return motion_state_sub && motion_state_sub->get_matched() > 0;
    case Endpoint::ControlGroupStateSubscriber:
      return control_group_state_sub &&
             control_group_state_sub->get_matched() > 0;
    case Endpoint::MotorConfigurationStateSubscriber:
      return motor_configuration_state_sub &&
             motor_configuration_state_sub->get_matched() > 0;
    case Endpoint::EstimatorDataSubscriber:
      return estimator_data_sub && estimator_data_sub->get_matched() > 0;
    case Endpoint::ErrorCodesSubscriber:
      return error_codes_sub && error_codes_sub->get_matched() > 0;
    }
    throw InvalidArgumentError("invalid Endpoint value");
  }

  EndpointMatchStatus status() const {
    EndpointMatchStatus status;
    std::lock_guard<std::mutex> lock(mutex);
    for (const auto endpoint : options.public_options.enabled_endpoints) {
      auto &endpoint_status = endpointStatus(status, endpoint);
      endpoint_status.enabled = true;
      endpoint_status.matched = accepting && matched(endpoint);
    }
    return status;
  }

  template <typename Handler>
  void setHandler(Endpoint endpoint, Handler Impl::*handler_member,
                  Handler handler) {
    std::lock_guard<std::mutex> lock(mutex);
    if (!options.endpointEnabled(endpoint)) {
      throw InvalidStateError(std::string(endpointName(endpoint)) +
                              " is disabled");
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
    std::unique_ptr<ManageLeaseClient> detached_manage_lease_service;
    std::unique_ptr<SubControllerClient> detached_sub_controller_service;
    std::unique_ptr<MotionCommandPublisher> detached_motion_command_pub;
    std::unique_ptr<ControlGroupCommandPublisher>
        detached_control_group_command_pub;
    std::unique_ptr<MotorConfigurationCommandPublisher>
        detached_motor_configuration_command_pub;
    std::unique_ptr<AuroraStateSubscription> detached_aurora_state_sub;
    std::unique_ptr<MotionStateSubscription> detached_motion_state_sub;
    std::unique_ptr<ControlGroupStateSubscription>
        detached_control_group_state_sub;
    std::unique_ptr<MotorConfigurationStateSubscription>
        detached_motor_configuration_state_sub;
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
      motor_configuration_state_handler = nullptr;
      estimator_data_handler = nullptr;
      error_codes_handler = nullptr;

      detached_error_codes_sub = std::move(error_codes_sub);
      detached_estimator_data_sub = std::move(estimator_data_sub);
      detached_control_group_state_sub = std::move(control_group_state_sub);
      detached_motor_configuration_state_sub =
          std::move(motor_configuration_state_sub);
      detached_motion_state_sub = std::move(motion_state_sub);
      detached_aurora_state_sub = std::move(aurora_state_sub);
      detached_motor_configuration_command_pub =
          std::move(motor_configuration_command_pub);
      detached_control_group_command_pub = std::move(control_group_command_pub);
      detached_motion_command_pub = std::move(motion_command_pub);
      detached_fsm_service = std::move(fsm_service);
      detached_manage_lease_service = std::move(manage_lease_service);
      detached_sub_controller_service = std::move(sub_controller_service);
      detached_node = std::move(node);
    }

    detached_error_codes_sub.reset();
    detached_estimator_data_sub.reset();
    detached_control_group_state_sub.reset();
    detached_motor_configuration_state_sub.reset();
    detached_motion_state_sub.reset();
    detached_aurora_state_sub.reset();
    detached_motor_configuration_command_pub.reset();
    detached_control_group_command_pub.reset();
    detached_motion_command_pub.reset();
    detached_fsm_service.reset();
    detached_manage_lease_service.reset();
    detached_sub_controller_service.reset();
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
        std::cerr << "[fourier_aurora_sdk] internal terminal observer failed"
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
  std::unique_ptr<ManageLeaseClient> manage_lease_service;
  std::unique_ptr<SubControllerClient> sub_controller_service;
  std::unique_ptr<MotionCommandPublisher> motion_command_pub;
  std::unique_ptr<ControlGroupCommandPublisher> control_group_command_pub;
  std::unique_ptr<MotorConfigurationCommandPublisher>
      motor_configuration_command_pub;
  std::unique_ptr<AuroraStateSubscription> aurora_state_sub;
  std::unique_ptr<MotionStateSubscription> motion_state_sub;
  std::unique_ptr<ControlGroupStateSubscription> control_group_state_sub;
  std::unique_ptr<MotorConfigurationStateSubscription>
      motor_configuration_state_sub;
  std::unique_ptr<EstimatorDataSubscription> estimator_data_sub;
  std::unique_ptr<ErrorCodesSubscription> error_codes_sub;

  AuroraStateHandler aurora_state_handler;
  MotionStateHandler motion_state_handler;
  ControlGroupStateHandler control_group_state_handler;
  MotorConfigurationStateHandler motor_configuration_state_handler;
  EstimatorDataHandler estimator_data_handler;
  ErrorCodesHandler error_codes_handler;
  detail::TransportTerminalObserver terminal_observer;
  bool callbacks_are_user = true;
};

FourierDdsTransport::FourierDdsTransport(ConnectionOptions options)
    : FourierDdsTransport(std::move(options), InitialHandlers{}) {}

FourierDdsTransport::FourierDdsTransport(ConnectionOptions options,
                                         InitialHandlers handlers)
    : impl_(Impl::create(options, std::move(handlers))) {}

FourierDdsTransport::~FourierDdsTransport() noexcept {
  if (!impl_) {
    return;
  }
  try {
    if (detail::inUserCallback()) {
      std::cerr << "[fourier_aurora_sdk] FourierDdsTransport destroyed from "
                   "callback context; teardown was deferred"
                << std::endl;
      (void)impl_->requestStop();
    } else {
      impl_->requestStop().get();
    }
  } catch (const std::exception &error) {
    std::cerr << "[fourier_aurora_sdk] transport destructor cleanup failed: "
              << error.what() << std::endl;
  } catch (...) {
    std::cerr << "[fourier_aurora_sdk] transport destructor cleanup failed"
              << std::endl;
  }
  impl_.reset();
}

void FourierDdsTransport::setAuroraStateHandler(AuroraStateHandler handler) {
  impl_->setHandler(Endpoint::AuroraStateSubscriber,
                    &Impl::aurora_state_handler, std::move(handler));
}

void FourierDdsTransport::setMotionStateHandler(MotionStateHandler handler) {
  impl_->setHandler(Endpoint::MotionStateSubscriber,
                    &Impl::motion_state_handler, std::move(handler));
}

void FourierDdsTransport::setControlGroupStateHandler(
    ControlGroupStateHandler handler) {
  impl_->setHandler(Endpoint::ControlGroupStateSubscriber,
                    &Impl::control_group_state_handler, std::move(handler));
}

void FourierDdsTransport::setMotorConfigurationStateHandler(
    MotorConfigurationStateHandler handler) {
  impl_->setHandler(Endpoint::MotorConfigurationStateSubscriber,
                    &Impl::motor_configuration_state_handler,
                    std::move(handler));
}

void FourierDdsTransport::setEstimatorDataHandler(
    EstimatorDataHandler handler) {
  impl_->setHandler(Endpoint::EstimatorDataSubscriber,
                    &Impl::estimator_data_handler, std::move(handler));
}

void FourierDdsTransport::setErrorCodesHandler(ErrorCodesHandler handler) {
  impl_->setHandler(Endpoint::ErrorCodesSubscriber, &Impl::error_codes_handler,
                    std::move(handler));
}

OperationResult
FourierDdsTransport::publishMotionCommand(const MotionCommand &command,
                                          const detail::LeaseToken &lease) {
  return impl_->publish(
      Endpoint::MotionCommandPublisher, impl_->motion_command_pub.get(),
      [&command, &lease] { return convertMotionCommand(command, lease); });
}

OperationResult FourierDdsTransport::publishControlGroupCommand(
    const ControlGroupCommand &command, const detail::LeaseToken &lease) {
  return impl_->publish(Endpoint::ControlGroupCommandPublisher,
                        impl_->control_group_command_pub.get(),
                        [&command, &lease] {
                          return convertControlGroupCommand(command, lease);
                        });
}

OperationResult FourierDdsTransport::publishMotorConfigurationCommand(
    const MotorConfigurationCommand &command, const detail::LeaseToken &lease) {
  return impl_->publish(
      Endpoint::MotorConfigurationCommandPublisher,
      impl_->motor_configuration_command_pub.get(), [&command, &lease] {
        return convertMotorConfigurationCommand(command, lease);
      });
}

OperationResult
FourierDdsTransport::setSubController(const SubControllerCommand &command) {
  return impl_->setSubController(command);
}

detail::LeaseManagementResponse
FourierDdsTransport::manageLease(const detail::LeaseManagementRequest &request,
                                 uint32_t timeout_ms) {
  return impl_->manageLease(request, timeout_ms);
}

OperationResult FourierDdsTransport::requestFsmState(uint32_t state_id) {
  return requestFsmState(state_id, false);
}

OperationResult FourierDdsTransport::requestFsmState(uint32_t state_id,
                                                     bool force_switch) {
  return impl_->requestFsmState(state_id, force_switch);
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

bool FourierDdsTransport::isMotorConfigurationCommandPublisherMatched() const {
  return endpointMatchStatus().motor_configuration_command_publisher.matched;
}

bool FourierDdsTransport::isSubControllerServiceMatched() const {
  return endpointMatchStatus().sub_controller_service.matched;
}

bool FourierDdsTransport::isManageLeaseServiceMatched() const {
  return endpointMatchStatus().manage_lease_service.matched;
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

bool FourierDdsTransport::isMotorConfigurationStateSubscriberMatched() const {
  return endpointMatchStatus().motor_configuration_state_subscriber.matched;
}

bool FourierDdsTransport::isEstimatorDataSubscriberMatched() const {
  return endpointMatchStatus().estimator_data_subscriber.matched;
}

bool FourierDdsTransport::isErrorCodesSubscriberMatched() const {
  return endpointMatchStatus().error_codes_subscriber.matched;
}

void FourierDdsTransport::stop() {
  if (detail::inUserCallback()) {
    throw InvalidStateError("stop() must not be called from an SDK callback");
  }
  try {
    impl_->requestStop().get();
  } catch (const ConnectionError &) {
    throw;
  } catch (const std::exception &error) {
    throw ConnectionError(std::string("transport teardown failed: ") +
                          error.what());
  } catch (...) {
    throw ConnectionError("transport teardown failed");
  }
}

#else

struct FourierDdsTransport::Impl {};

FourierDdsTransport::FourierDdsTransport(ConnectionOptions options)
    : FourierDdsTransport(std::move(options), InitialHandlers{}) {}

FourierDdsTransport::FourierDdsTransport(ConnectionOptions options,
                                         InitialHandlers) {
  (void)detail::normalizeConnectionOptions(options);
  throw ConnectionError("FourierDDS transport is disabled in this SDK build");
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
void FourierDdsTransport::setMotorConfigurationStateHandler(
    MotorConfigurationStateHandler) {
  throw InvalidStateError("transport session is stopped");
}
void FourierDdsTransport::setEstimatorDataHandler(EstimatorDataHandler) {
  throw InvalidStateError("transport session is stopped");
}
void FourierDdsTransport::setErrorCodesHandler(ErrorCodesHandler) {
  throw InvalidStateError("transport session is stopped");
}

OperationResult
FourierDdsTransport::publishMotionCommand(const MotionCommand &,
                                          const detail::LeaseToken &) {
  return OperationResult{
      result_codes::CLIENT_NOT_RUNNING,
      std::string(defaultResultMessage(result_codes::CLIENT_NOT_RUNNING)),
  };
}
OperationResult
FourierDdsTransport::publishControlGroupCommand(const ControlGroupCommand &,
                                                const detail::LeaseToken &) {
  return OperationResult{
      result_codes::CLIENT_NOT_RUNNING,
      std::string(defaultResultMessage(result_codes::CLIENT_NOT_RUNNING)),
  };
}
OperationResult FourierDdsTransport::publishMotorConfigurationCommand(
    const MotorConfigurationCommand &, const detail::LeaseToken &) {
  return OperationResult{
      result_codes::CLIENT_NOT_RUNNING,
      std::string(defaultResultMessage(result_codes::CLIENT_NOT_RUNNING)),
  };
}
OperationResult
FourierDdsTransport::setSubController(const SubControllerCommand &) {
  return OperationResult{
      result_codes::CLIENT_NOT_RUNNING,
      std::string(defaultResultMessage(result_codes::CLIENT_NOT_RUNNING)),
  };
}
detail::LeaseManagementResponse
FourierDdsTransport::manageLease(const detail::LeaseManagementRequest &,
                                 uint32_t) {
  detail::LeaseManagementResponse response;
  response.operation = OperationResult{
      result_codes::CLIENT_NOT_RUNNING,
      std::string(defaultResultMessage(result_codes::CLIENT_NOT_RUNNING)),
  };
  return response;
}
OperationResult FourierDdsTransport::requestFsmState(uint32_t state_id) {
  return requestFsmState(state_id, false);
}
OperationResult FourierDdsTransport::requestFsmState(uint32_t, bool) {
  return OperationResult{
      result_codes::CLIENT_NOT_RUNNING,
      std::string(defaultResultMessage(result_codes::CLIENT_NOT_RUNNING)),
  };
}
EndpointMatchStatus FourierDdsTransport::endpointMatchStatus() const {
  return {};
}
bool FourierDdsTransport::isFsmServiceMatched() const { return false; }
bool FourierDdsTransport::isMotionCommandPublisherMatched() const {
  return false;
}
bool FourierDdsTransport::isControlGroupCommandPublisherMatched() const {
  return false;
}
bool FourierDdsTransport::isMotorConfigurationCommandPublisherMatched() const {
  return false;
}
bool FourierDdsTransport::isSubControllerServiceMatched() const {
  return false;
}
bool FourierDdsTransport::isManageLeaseServiceMatched() const { return false; }
bool FourierDdsTransport::isAuroraStateSubscriberMatched() const {
  return false;
}
bool FourierDdsTransport::isMotionStateSubscriberMatched() const {
  return false;
}
bool FourierDdsTransport::isControlGroupStateSubscriberMatched() const {
  return false;
}
bool FourierDdsTransport::isMotorConfigurationStateSubscriberMatched() const {
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

} // namespace

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

  OperationResult publishMotionCommand(const MotionCommand &command,
                                       const LeaseToken &lease) override {
    return transport_->publishMotionCommand(command, lease);
  }

  OperationResult publishControlGroupCommand(const ControlGroupCommand &command,
                                             const LeaseToken &lease) override {
    return transport_->publishControlGroupCommand(command, lease);
  }

  OperationResult
  publishMotorConfigurationCommand(const MotorConfigurationCommand &command,
                                   const LeaseToken &lease) override {
    return transport_->publishMotorConfigurationCommand(command, lease);
  }

  OperationResult
  setSubController(const SubControllerCommand &command) override {
    return transport_->setSubController(command);
  }

  LeaseManagementResponse manageLease(const LeaseManagementRequest &request,
                                      uint32_t timeout_ms) override {
    return transport_->manageLease(request, timeout_ms);
  }

  OperationResult requestFsmState(uint32_t state_id,
                                  bool force_switch) override {
    return transport_->requestFsmState(state_id, force_switch);
  }

  EndpointMatchStatus endpointMatchStatus() const override {
    return transport_->endpointMatchStatus();
  }

  void stop() override { transport_->stop(); }

private:
  std::unique_ptr<FourierDdsTransport> transport_;
};

class FourierDdsTransportFactory {
public:
  static std::unique_ptr<ITransport>
  create(const ConnectionOptions &options, TransportHandlers handlers,
         TransportTerminalObserver terminal_observer) {
    FourierDdsTransport::InitialHandlers initial;
    initial.callbacks = std::move(handlers);
    initial.terminal_observer = std::move(terminal_observer);
    initial.callbacks_are_user = false;
    auto transport = std::unique_ptr<FourierDdsTransport>(
        new FourierDdsTransport(options, std::move(initial)));
    return std::make_unique<FourierDdsTransportAdapter>(std::move(transport));
  }
};

std::unique_ptr<ITransport>
makeProductionTransport(const ConnectionOptions &options,
                        TransportHandlers handlers,
                        TransportTerminalObserver terminal_observer) {
#if defined(FOURIER_AURORA_SDK_TESTING)
  TransportFactory factory;
  {
    std::lock_guard<std::mutex> lock(test_factory_mutex);
    factory = test_factory;
  }
  if (factory) {
    return factory(options, std::move(handlers), std::move(terminal_observer));
  }
#endif
  return FourierDdsTransportFactory::create(options, std::move(handlers),
                                            std::move(terminal_observer));
}

} // namespace detail

} // namespace fourier::aurora_sdk
