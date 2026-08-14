#include <pybind11/chrono.h>
#include <pybind11/pybind11.h>
#include <pybind11/stl.h>

#include "fourier_aurora_sdk/errors.hpp"
#include "fourier_aurora_sdk/types.hpp"

namespace py = pybind11;
namespace sdk = fourier::aurora_sdk;

void bind_types(py::module_& m) {
    const auto aurora_sdk_error = py::register_exception<sdk::AuroraSdkError>(
        m, "AuroraSdkError", PyExc_RuntimeError);
    py::register_exception<sdk::ConnectionError>(
        m, "ConnectionError", aurora_sdk_error.ptr());
    py::register_exception<sdk::InvalidArgumentError>(
        m, "InvalidArgumentError", aurora_sdk_error.ptr());
    py::register_exception<sdk::InvalidStateError>(
        m, "InvalidStateError", aurora_sdk_error.ptr());

    py::enum_<sdk::ClientState>(m, "ClientState")
        .value("STOPPED", sdk::ClientState::Stopped)
        .value("STARTING", sdk::ClientState::Starting)
        .value("RUNNING", sdk::ClientState::Running)
        .value("STOPPING", sdk::ClientState::Stopping);

    py::enum_<sdk::Endpoint>(m, "Endpoint")
        .value("FSM_SERVICE", sdk::Endpoint::FsmService)
        .value("MANAGE_LEASE_SERVICE", sdk::Endpoint::ManageLeaseService)
        .value("MOTION_COMMAND_PUBLISHER", sdk::Endpoint::MotionCommandPublisher)
        .value(
            "CONTROL_GROUP_COMMAND_PUBLISHER",
            sdk::Endpoint::ControlGroupCommandPublisher)
        .value(
            "MOTOR_CONFIGURATION_COMMAND_PUBLISHER",
            sdk::Endpoint::MotorConfigurationCommandPublisher)
        .value("SUB_CONTROLLER_SERVICE", sdk::Endpoint::SubControllerService)
        .value("AURORA_STATE_SUBSCRIBER", sdk::Endpoint::AuroraStateSubscriber)
        .value("MOTION_STATE_SUBSCRIBER", sdk::Endpoint::MotionStateSubscriber)
        .value(
            "CONTROL_GROUP_STATE_SUBSCRIBER",
            sdk::Endpoint::ControlGroupStateSubscriber)
        .value(
            "MOTOR_CONFIGURATION_STATE_SUBSCRIBER",
            sdk::Endpoint::MotorConfigurationStateSubscriber)
        .value("ESTIMATOR_DATA_SUBSCRIBER", sdk::Endpoint::EstimatorDataSubscriber)
        .value("ERROR_CODES_SUBSCRIBER", sdk::Endpoint::ErrorCodesSubscriber);

    py::class_<sdk::EndpointStatus>(m, "EndpointStatus")
        .def(py::init<>())
        .def_readwrite("enabled", &sdk::EndpointStatus::enabled)
        .def_readwrite("matched", &sdk::EndpointStatus::matched);

    py::class_<sdk::EndpointMatchStatus>(m, "EndpointMatchStatus")
        .def(py::init<>())
        .def_readwrite("fsm_service", &sdk::EndpointMatchStatus::fsm_service)
        .def_readwrite("manage_lease_service", &sdk::EndpointMatchStatus::manage_lease_service)
        .def_readwrite(
            "motion_command_publisher",
            &sdk::EndpointMatchStatus::motion_command_publisher)
        .def_readwrite(
            "control_group_command_publisher",
            &sdk::EndpointMatchStatus::control_group_command_publisher)
        .def_readwrite(
            "motor_configuration_command_publisher",
            &sdk::EndpointMatchStatus::motor_configuration_command_publisher)
        .def_readwrite("sub_controller_service", &sdk::EndpointMatchStatus::sub_controller_service)
        .def_readwrite(
            "aurora_state_subscriber",
            &sdk::EndpointMatchStatus::aurora_state_subscriber)
        .def_readwrite(
            "motion_state_subscriber",
            &sdk::EndpointMatchStatus::motion_state_subscriber)
        .def_readwrite(
            "control_group_state_subscriber",
            &sdk::EndpointMatchStatus::control_group_state_subscriber)
        .def_readwrite(
            "motor_configuration_state_subscriber",
            &sdk::EndpointMatchStatus::motor_configuration_state_subscriber)
        .def_readwrite(
            "estimator_data_subscriber",
            &sdk::EndpointMatchStatus::estimator_data_subscriber)
        .def_readwrite(
            "error_codes_subscriber",
            &sdk::EndpointMatchStatus::error_codes_subscriber);

    py::class_<sdk::OperationResult>(m, "OperationResult")
        .def(py::init<>())
        .def_readwrite("code", &sdk::OperationResult::code)
        .def_readwrite("message", &sdk::OperationResult::message)
        .def("success", &sdk::OperationResult::success);

    m.def(
        "result_code_name",
        [](uint32_t code) { return std::string(sdk::resultCodeName(code)); });
    m.def(
        "default_result_message",
        [](uint32_t code) { return std::string(sdk::defaultResultMessage(code)); });
    m.attr("SUCCESS") = sdk::result_codes::SUCCESS;
    m.attr("CLIENT_ERROR") = sdk::result_codes::CLIENT_ERROR;
    m.attr("CLIENT_NOT_RUNNING") = sdk::result_codes::CLIENT_NOT_RUNNING;
    m.attr("ENDPOINT_DISABLED") = sdk::result_codes::ENDPOINT_DISABLED;
    m.attr("ENDPOINT_UNMATCHED") = sdk::result_codes::ENDPOINT_UNMATCHED;
    m.attr("PUBLISH_FAILED") = sdk::result_codes::PUBLISH_FAILED;
    m.attr("SERVICE_TIMEOUT") = sdk::result_codes::SERVICE_TIMEOUT;
    m.attr("TRANSPORT_ERROR") = sdk::result_codes::TRANSPORT_ERROR;
    m.attr("INVALID_AURORA_RESPONSE") = sdk::result_codes::INVALID_AURORA_RESPONSE;
    m.attr("STATE_NOT_RECEIVED") = sdk::result_codes::STATE_NOT_RECEIVED;
    m.attr("LEASE_NOT_ACTIVE") = sdk::result_codes::LEASE_NOT_ACTIVE;
    m.attr("LEASE_RESOURCE_NOT_COVERED") = sdk::result_codes::LEASE_RESOURCE_NOT_COVERED;
    m.attr("LEASE_LOST") = sdk::result_codes::LEASE_LOST;

    py::class_<sdk::MessageHeader>(m, "MessageHeader")
        .def(py::init<>())
        .def_readwrite("send_time", &sdk::MessageHeader::send_time)
        .def_readwrite("frame_id", &sdk::MessageHeader::frame_id)
        .def_readwrite("received_time", &sdk::MessageHeader::received_time);

    py::class_<sdk::StateInfo>(m, "StateInfo")
        .def(py::init<>())
        .def_readwrite("id", &sdk::StateInfo::id)
        .def_readwrite("version", &sdk::StateInfo::version)
        .def_readwrite("name", &sdk::StateInfo::name)
        .def_readwrite("description", &sdk::StateInfo::description)
        .def_readwrite("active", &sdk::StateInfo::active);

    py::class_<sdk::RobotInfo>(m, "RobotInfo")
        .def(py::init<>())
        .def_readwrite("robot_type", &sdk::RobotInfo::robot_type)
        .def_readwrite("hardware_type", &sdk::RobotInfo::hardware_type)
        .def_readwrite("end_effector_type", &sdk::RobotInfo::end_effector_type);

    py::class_<sdk::AuroraState>(m, "AuroraState")
        .def(py::init<>())
        .def_readwrite("header", &sdk::AuroraState::header)
        .def_readwrite("robot_info", &sdk::AuroraState::robot_info)
        .def_readwrite("switch_status", &sdk::AuroraState::switch_status)
        .def_readwrite("current_state", &sdk::AuroraState::current_state)
        .def_readwrite("available_states", &sdk::AuroraState::available_states)
        .def_readwrite("authority_resources", &sdk::AuroraState::authority_resources);

    py::class_<sdk::MotionState>(m, "MotionState")
        .def(py::init<>())
        .def_readwrite("header", &sdk::MotionState::header)
        .def_readwrite("position", &sdk::MotionState::position)
        .def_readwrite("position_cmd", &sdk::MotionState::position_cmd)
        .def_readwrite("lin_velocity", &sdk::MotionState::lin_velocity)
        .def_readwrite("lin_velocity_cmd", &sdk::MotionState::lin_velocity_cmd)
        .def_readwrite("attitude", &sdk::MotionState::attitude)
        .def_readwrite("attitude_cmd", &sdk::MotionState::attitude_cmd)
        .def_readwrite("ang_velocity", &sdk::MotionState::ang_velocity)
        .def_readwrite("ang_velocity_cmd", &sdk::MotionState::ang_velocity_cmd)
        .def_readwrite("delta_height", &sdk::MotionState::delta_height)
        .def_readwrite("delta_height_cmd", &sdk::MotionState::delta_height_cmd)
        .def_readwrite("progress", &sdk::MotionState::progress)
        .def_readwrite("stable_level", &sdk::MotionState::stable_level);

    py::class_<sdk::MotionCommand>(m, "MotionCommand")
        .def(py::init<>())
        .def_readwrite("header", &sdk::MotionCommand::header)
        .def_readwrite("position", &sdk::MotionCommand::position)
        .def_readwrite("lin_velocity", &sdk::MotionCommand::lin_velocity)
        .def_readwrite("attitude", &sdk::MotionCommand::attitude)
        .def_readwrite("ang_velocity", &sdk::MotionCommand::ang_velocity)
        .def_readwrite("delta_height", &sdk::MotionCommand::delta_height)
        .def_readwrite("gait_type", &sdk::MotionCommand::gait_type)
        .def_readwrite("skills", &sdk::MotionCommand::skills)
        .def_readwrite("duration", &sdk::MotionCommand::duration);

    py::enum_<sdk::JointField>(m, "JointField")
        .value("POSITION", sdk::JointField::Position)
        .value("VELOCITY", sdk::JointField::Velocity)
        .value("EFFORT", sdk::JointField::Effort);

    py::enum_<sdk::JointControlType>(m, "JointControlType")
        .value("JOINT", sdk::JointControlType::Joint)
        .value("MOTOR", sdk::JointControlType::Motor);

    py::enum_<sdk::MotorMode>(m, "MotorMode")
        .value("PD", sdk::MotorMode::Pd)
        .value("POSITION", sdk::MotorMode::Position)
        .value("EFFORT", sdk::MotorMode::Effort);

    py::class_<sdk::ControlGroupStateEntry>(m, "ControlGroupStateEntry")
        .def(py::init<>())
        .def_readwrite("group_name", &sdk::ControlGroupStateEntry::group_name)
        .def_readwrite("joint_position", &sdk::ControlGroupStateEntry::joint_position)
        .def_readwrite("joint_velocity", &sdk::ControlGroupStateEntry::joint_velocity)
        .def_readwrite("joint_effort", &sdk::ControlGroupStateEntry::joint_effort);

    py::class_<sdk::CartesianStateEntry>(m, "CartesianStateEntry")
        .def(py::init<>())
        .def_readwrite("link_name", &sdk::CartesianStateEntry::link_name)
        .def_readwrite("pose", &sdk::CartesianStateEntry::pose)
        .def_readwrite("twist", &sdk::CartesianStateEntry::twist)
        .def_readwrite("wrench", &sdk::CartesianStateEntry::wrench);

    py::class_<sdk::ControlGroupState>(m, "ControlGroupState")
        .def(py::init<>())
        .def_readwrite("header", &sdk::ControlGroupState::header)
        .def_readwrite("joint_state", &sdk::ControlGroupState::joint_state)
        .def_readwrite("joint_cmd", &sdk::ControlGroupState::joint_cmd)
        .def_readwrite("cartesian_state", &sdk::ControlGroupState::cartesian_state)
        .def_readwrite("cartesian_cmd", &sdk::ControlGroupState::cartesian_cmd);

    py::class_<sdk::JointCommand>(m, "JointCommand")
        .def(py::init<>())
        .def_readwrite("group_name", &sdk::JointCommand::group_name)
        .def_readwrite("control_type", &sdk::JointCommand::control_type)
        .def_readwrite("motor_mode", &sdk::JointCommand::motor_mode)
        .def_readwrite("position", &sdk::JointCommand::position)
        .def_readwrite("velocity", &sdk::JointCommand::velocity)
        .def_readwrite("effort", &sdk::JointCommand::effort);

    py::class_<sdk::ControlGroupCommand>(m, "ControlGroupCommand")
        .def(py::init<>())
        .def_readwrite("header", &sdk::ControlGroupCommand::header)
        .def_readwrite("joint_cmd", &sdk::ControlGroupCommand::joint_cmd);

    py::class_<sdk::BaseData>(m, "BaseData")
        .def(py::init<>())
        .def_readwrite("header", &sdk::BaseData::header)
        .def_readwrite("quat_xyzw", &sdk::BaseData::quat_xyzw)
        .def_readwrite("quat_wxyz", &sdk::BaseData::quat_wxyz)
        .def_readwrite("rpy", &sdk::BaseData::rpy)
        .def_readwrite("omega_W", &sdk::BaseData::omega_W)
        .def_readwrite("acc_W", &sdk::BaseData::acc_W)
        .def_readwrite("omega_B", &sdk::BaseData::omega_B)
        .def_readwrite("acc_B", &sdk::BaseData::acc_B)
        .def_readwrite("vel_W", &sdk::BaseData::vel_W)
        .def_readwrite("pos_W", &sdk::BaseData::pos_W)
        .def_readwrite("vel_B", &sdk::BaseData::vel_B)
        .def_readwrite("stable_level", &sdk::BaseData::stable_level);

    py::class_<sdk::ContactData>(m, "ContactData")
        .def(py::init<>())
        .def_readwrite("header", &sdk::ContactData::header)
        .def_readwrite("contact_force", &sdk::ContactData::contact_force)
        .def_readwrite("contact_prob", &sdk::ContactData::contact_prob);

    py::class_<sdk::EstimatorData>(m, "EstimatorData")
        .def(py::init<>())
        .def_readwrite("header", &sdk::EstimatorData::header)
        .def_readwrite("quat_xyzw", &sdk::EstimatorData::quat_xyzw)
        .def_readwrite("quat_wxyz", &sdk::EstimatorData::quat_wxyz)
        .def_readwrite("rpy", &sdk::EstimatorData::rpy)
        .def_readwrite("omega_W", &sdk::EstimatorData::omega_W)
        .def_readwrite("acc_W", &sdk::EstimatorData::acc_W)
        .def_readwrite("omega_B", &sdk::EstimatorData::omega_B)
        .def_readwrite("acc_B", &sdk::EstimatorData::acc_B)
        .def_readwrite("vel_W", &sdk::EstimatorData::vel_W)
        .def_readwrite("pos_W", &sdk::EstimatorData::pos_W)
        .def_readwrite("vel_B", &sdk::EstimatorData::vel_B)
        .def_readwrite("contact_force", &sdk::EstimatorData::contact_force)
        .def_readwrite("contact_prob", &sdk::EstimatorData::contact_prob)
        .def_readwrite("stable_level", &sdk::EstimatorData::stable_level);

    py::class_<sdk::GroupOverrideParam>(m, "GroupOverrideParam")
        .def(py::init<>())
        .def_readwrite("group_name", &sdk::GroupOverrideParam::group_name)
        .def_readwrite("control_param", &sdk::GroupOverrideParam::control_param);

    py::class_<sdk::ControllerParam>(m, "ControllerParam")
        .def(py::init<>())
        .def_readwrite("controller_name", &sdk::ControllerParam::controller_name)
        .def_readwrite("rate", &sdk::ControllerParam::rate)
        .def_readwrite("group_override_params", &sdk::ControllerParam::group_override_params);

    py::class_<sdk::SubControllerCommand>(m, "SubControllerCommand")
        .def(py::init<>())
        .def_readwrite("header", &sdk::SubControllerCommand::header)
        .def_readwrite("reset", &sdk::SubControllerCommand::reset)
        .def_readwrite("controller_params", &sdk::SubControllerCommand::controller_params);

    py::class_<sdk::ErrorCodeEntry>(m, "ErrorCodeEntry")
        .def(py::init<>())
        .def_readwrite("high32", &sdk::ErrorCodeEntry::high32)
        .def_readwrite("low32", &sdk::ErrorCodeEntry::low32);

    py::class_<sdk::ErrorCodes>(m, "ErrorCodes")
        .def(py::init<>())
        .def_readwrite("header", &sdk::ErrorCodes::header)
        .def_readwrite("source", &sdk::ErrorCodes::source)
        .def_readwrite("error_codes", &sdk::ErrorCodes::error_codes);

    py::enum_<sdk::AuroraResource>(m, "AuroraResource")
        .value("LEFT_LEG", sdk::AuroraResource::LeftLeg)
        .value("RIGHT_LEG", sdk::AuroraResource::RightLeg)
        .value("WAIST", sdk::AuroraResource::Waist)
        .value("HEAD", sdk::AuroraResource::Head)
        .value("LEFT_MANIPULATOR", sdk::AuroraResource::LeftManipulator)
        .value("RIGHT_MANIPULATOR", sdk::AuroraResource::RightManipulator)
        .value("LEFT_END_EFFECTOR", sdk::AuroraResource::LeftEndEffector)
        .value("RIGHT_END_EFFECTOR", sdk::AuroraResource::RightEndEffector)
        .value("MOTION_COMMAND", sdk::AuroraResource::MotionCommand);

    py::enum_<sdk::LeasePriority>(m, "LeasePriority")
        .value("NORMAL", sdk::LeasePriority::Normal)
        .value("EMERGENCY", sdk::LeasePriority::Emergency);
    py::enum_<sdk::LeaseState>(m, "LeaseState")
        .value("UNREGISTERED", sdk::LeaseState::Unregistered)
        .value("REGISTERING", sdk::LeaseState::Registering)
        .value("ACTIVE", sdk::LeaseState::Active)
        .value("REPLACING", sdk::LeaseState::Replacing)
        .value("RELEASING", sdk::LeaseState::Releasing)
        .value("LOST", sdk::LeaseState::Lost);
    py::enum_<sdk::LeaseLossReason>(m, "LeaseLossReason")
        .value("NONE", sdk::LeaseLossReason::None)
        .value("DEADLINE_EXPIRED", sdk::LeaseLossReason::DeadlineExpired)
        .value("AUTHORITY_REJECTED", sdk::LeaseLossReason::AuthorityRejected)
        .value("OPERATION_TIMEOUT", sdk::LeaseLossReason::OperationTimeout)
        .value("INVALID_AUTHORITY_RESPONSE", sdk::LeaseLossReason::InvalidAuthorityResponse)
        .value("PUBLICATION_DRAIN_TIMEOUT", sdk::LeaseLossReason::PublicationDrainTimeout)
        .value("INTERNAL_ERROR", sdk::LeaseLossReason::InternalError);
    py::class_<sdk::LeaseOption>(m, "LeaseOption")
        .def(py::init<>())
        .def_readwrite("duration", &sdk::LeaseOption::duration)
        .def_readwrite("priority", &sdk::LeaseOption::priority)
        .def_readwrite("timeout", &sdk::LeaseOption::timeout);
    py::class_<sdk::LeaseResult>(m, "LeaseResult")
        .def(py::init<>())
        .def_readwrite("operation", &sdk::LeaseResult::operation)
        .def_readwrite("state", &sdk::LeaseResult::state)
        .def_readwrite("source_name", &sdk::LeaseResult::source_name)
        .def_readwrite("lease_id", &sdk::LeaseResult::lease_id)
        .def_readwrite("resources", &sdk::LeaseResult::resources)
        .def_readwrite("priority", &sdk::LeaseResult::priority)
        .def_readwrite("granted_duration", &sdk::LeaseResult::granted_duration)
        .def_readwrite("remaining_duration", &sdk::LeaseResult::remaining_duration)
        .def_readwrite("loss_reason", &sdk::LeaseResult::loss_reason)
        .def("success", &sdk::LeaseResult::success)
        .def("usable", &sdk::LeaseResult::usable);
    py::class_<sdk::AuthorityResourceStatus>(m, "AuthorityResourceStatus")
        .def(py::init<>())
        .def_readwrite("resource", &sdk::AuthorityResourceStatus::resource)
        .def_readwrite("acquired", &sdk::AuthorityResourceStatus::acquired)
        .def_readwrite("holder_source_name", &sdk::AuthorityResourceStatus::holder_source_name)
        .def_readwrite("holder_priority", &sdk::AuthorityResourceStatus::holder_priority);

    py::enum_<sdk::MotorConfigurationMode>(m, "MotorConfigurationMode")
        .value("DISABLE", sdk::MotorConfigurationMode::Disable)
        .value("POSITION", sdk::MotorConfigurationMode::Position)
        .value("VELOCITY", sdk::MotorConfigurationMode::Velocity)
        .value("TORQUE", sdk::MotorConfigurationMode::Torque)
        .value("PD", sdk::MotorConfigurationMode::Pd);
    py::class_<sdk::MotorConfigurationGroupCommand>(m, "MotorConfigurationGroupCommand")
        .def(py::init<>())
        .def_readwrite("group_name", &sdk::MotorConfigurationGroupCommand::group_name)
        .def_readwrite("mode", &sdk::MotorConfigurationGroupCommand::mode)
        .def_readwrite("position_kp", &sdk::MotorConfigurationGroupCommand::position_kp)
        .def_readwrite("position_ki", &sdk::MotorConfigurationGroupCommand::position_ki)
        .def_readwrite("position_kd", &sdk::MotorConfigurationGroupCommand::position_kd)
        .def_readwrite("pd_kp", &sdk::MotorConfigurationGroupCommand::pd_kp)
        .def_readwrite("pd_kd", &sdk::MotorConfigurationGroupCommand::pd_kd);
    py::class_<sdk::MotorConfigurationCommand>(m, "MotorConfigurationCommand")
        .def(py::init<>())
        .def_readwrite("header", &sdk::MotorConfigurationCommand::header)
        .def_readwrite("groups", &sdk::MotorConfigurationCommand::groups);
    py::class_<sdk::MotorConfigurationGroupState>(m, "MotorConfigurationGroupState")
        .def(py::init<>())
        .def_readwrite("group_name", &sdk::MotorConfigurationGroupState::group_name)
        .def_readwrite("mode", &sdk::MotorConfigurationGroupState::mode)
        .def_readwrite("motor_error", &sdk::MotorConfigurationGroupState::motor_error)
        .def_readwrite("position_kp", &sdk::MotorConfigurationGroupState::position_kp)
        .def_readwrite("position_ki", &sdk::MotorConfigurationGroupState::position_ki)
        .def_readwrite("position_kd", &sdk::MotorConfigurationGroupState::position_kd)
        .def_readwrite("pd_kp", &sdk::MotorConfigurationGroupState::pd_kp)
        .def_readwrite("pd_kd", &sdk::MotorConfigurationGroupState::pd_kd);
    py::class_<sdk::MotorConfigurationState>(m, "MotorConfigurationState")
        .def(py::init<>())
        .def_readwrite("header", &sdk::MotorConfigurationState::header)
        .def_readwrite("groups", &sdk::MotorConfigurationState::groups);

}
