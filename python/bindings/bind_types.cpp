#include <pybind11/chrono.h>
#include <pybind11/pybind11.h>
#include <pybind11/stl.h>

#include "fourier_aurora_sdk/errors.hpp"
#include "fourier_aurora_sdk/types.hpp"

namespace py = pybind11;
namespace sdk = fourier::aurora_sdk;

void bind_types(py::module_& m) {
    py::register_exception<sdk::AuroraSdkError>(m, "AuroraSdkError");
    py::register_exception<sdk::ConnectionError>(m, "ConnectionError");
    py::register_exception<sdk::EndpointNotReadyError>(m, "EndpointNotReadyError");
    py::register_exception<sdk::StateNotReadyError>(m, "StateNotReadyError");
    py::register_exception<sdk::PublishError>(m, "PublishError");
    py::register_exception<sdk::ServiceTimeoutError>(m, "ServiceTimeoutError");
    py::register_exception<sdk::InvalidArgumentError>(m, "InvalidArgumentError");

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
        .def_readwrite("available_states", &sdk::AuroraState::available_states);

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
        .value("Position", sdk::JointField::Position)
        .value("Velocity", sdk::JointField::Velocity)
        .value("Effort", sdk::JointField::Effort);

    py::enum_<sdk::JointControlType>(m, "JointControlType")
        .value("Joint", sdk::JointControlType::Joint)
        .value("Motor", sdk::JointControlType::Motor);

    py::enum_<sdk::MotorMode>(m, "MotorMode")
        .value("Pd", sdk::MotorMode::Pd)
        .value("Position", sdk::MotorMode::Position)
        .value("Effort", sdk::MotorMode::Effort);

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

    py::class_<sdk::CartesianCommand>(m, "CartesianCommand")
        .def(py::init<>())
        .def_readwrite("link_name", &sdk::CartesianCommand::link_name)
        .def_readwrite("pose", &sdk::CartesianCommand::pose)
        .def_readwrite("twist", &sdk::CartesianCommand::twist)
        .def_readwrite("wrench", &sdk::CartesianCommand::wrench);

    py::class_<sdk::ControlGroupCommand>(m, "ControlGroupCommand")
        .def(py::init<>())
        .def_readwrite("header", &sdk::ControlGroupCommand::header)
        .def_readwrite("joint_cmd", &sdk::ControlGroupCommand::joint_cmd)
        .def_readwrite("cartesian_cmd", &sdk::ControlGroupCommand::cartesian_cmd);

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

    py::class_<sdk::ServiceCallResult>(m, "ServiceCallResult")
        .def(py::init<>())
        .def_readwrite("result_code", &sdk::ServiceCallResult::result_code)
        .def_readwrite("message", &sdk::ServiceCallResult::message)
        .def_readwrite("timed_out", &sdk::ServiceCallResult::timed_out)
        .def_readwrite("transport_error", &sdk::ServiceCallResult::transport_error)
        .def("success", &sdk::ServiceCallResult::success);
}
