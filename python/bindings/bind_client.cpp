#include <pybind11/pybind11.h>
#include <pybind11/stl.h>

#include <chrono>
#include <cmath>
#include <cstdint>
#include <limits>
#include <memory>
#include <string>

#include "fourier_aurora_sdk/aurora_client.hpp"

namespace py = pybind11;
namespace sdk = fourier::aurora_sdk;

namespace {

uint32_t requireUint32(const py::handle& value, const char* field_name) {
    if (PyBool_Check(value.ptr()) || !PyLong_Check(value.ptr())) {
        throw sdk::InvalidArgumentError(
            std::string(field_name) + " must be an integer");
    }

    int overflow = 0;
    const long long converted =
        PyLong_AsLongLongAndOverflow(value.ptr(), &overflow);
    if (overflow != 0 || PyErr_Occurred()) {
        PyErr_Clear();
        throw sdk::InvalidArgumentError(
            std::string(field_name) + " must be in the range 0..4294967295");
    }
    if (converted < 0 ||
        static_cast<unsigned long long>(converted) >
            std::numeric_limits<uint32_t>::max()) {
        throw sdk::InvalidArgumentError(
            std::string(field_name) + " must be in the range 0..4294967295");
    }
    return static_cast<uint32_t>(converted);
}

uint8_t requireDdsMode(const py::handle& value) {
    const auto converted = requireUint32(value, "dds_mode");
    if (converted > 3) {
        throw sdk::InvalidArgumentError(
            "dds_mode must be one of 0, 1, 2, or 3");
    }
    return static_cast<uint8_t>(converted);
}

std::chrono::milliseconds requireWaitDuration(
    const py::handle& value,
    const char* field_name) {
    if (PyBool_Check(value.ptr()) ||
        (!PyLong_Check(value.ptr()) && !PyFloat_Check(value.ptr()))) {
        throw sdk::InvalidArgumentError(
            std::string(field_name) + " must be a finite number of seconds");
    }
    const double seconds = PyFloat_AsDouble(value.ptr());
    if (PyErr_Occurred()) {
        PyErr_Clear();
        throw sdk::InvalidArgumentError(
            std::string(field_name) + " must be a finite number of seconds");
    }
    if (!std::isfinite(seconds) || seconds < 0.0) {
        throw sdk::InvalidArgumentError(
            std::string(field_name) +
            " must be a finite nonnegative number of seconds");
    }

    const long double milliseconds =
        std::ceil(static_cast<long double>(seconds) * 1000.0L);
    const auto maximum = static_cast<long double>(
        std::chrono::milliseconds::max().count());
    if (milliseconds > maximum) {
        throw sdk::InvalidArgumentError(
            std::string(field_name) + " exceeds the native wait range");
    }
    return std::chrono::milliseconds(
        static_cast<std::chrono::milliseconds::rep>(milliseconds));
}

}  // namespace

void bind_client(py::module_& m) {
    py::class_<sdk::ConnectionOptions>(m, "ConnectionOptions")
        .def(py::init<>())
        .def_property(
            "domain_id",
            [](const sdk::ConnectionOptions& options) {
                return options.domain_id;
            },
            [](sdk::ConnectionOptions& options, const py::object& value) {
                options.domain_id = requireUint32(value, "domain_id");
            })
        .def_readwrite(
            "namespace_name",
            &sdk::ConnectionOptions::namespace_name)
        .def_readwrite(
            "topic_prefix",
            &sdk::ConnectionOptions::topic_prefix)
        .def_property(
            "dds_mode",
            [](const sdk::ConnectionOptions& options) {
                return options.dds_mode;
            },
            [](sdk::ConnectionOptions& options, const py::object& value) {
                options.dds_mode = requireDdsMode(value);
            })
        .def_readwrite(
            "is_ros_compatible",
            &sdk::ConnectionOptions::is_ros_compatible)
        .def_readwrite(
            "use_namespace",
            &sdk::ConnectionOptions::use_namespace)
        .def_readwrite(
            "use_discovery_server",
            &sdk::ConnectionOptions::use_discovery_server)
        .def_property(
            "service_timeout_ms",
            [](const sdk::ConnectionOptions& options) {
                return options.service_timeout_ms;
            },
            [](sdk::ConnectionOptions& options, const py::object& value) {
                options.service_timeout_ms =
                    requireUint32(value, "service_timeout_ms");
            })
        .def_property(
            "source_id",
            [](const sdk::ConnectionOptions& options) { return options.source_id; },
            [](sdk::ConnectionOptions& options, const py::object& value) {
                options.source_id = requireUint32(value, "source_id");
            })
        .def_readwrite("source_name", &sdk::ConnectionOptions::source_name)
        .def_readwrite(
            "allow_emergency_lease_priority",
            &sdk::ConnectionOptions::allow_emergency_lease_priority)
        .def_readwrite(
            "enabled_endpoints",
            &sdk::ConnectionOptions::enabled_endpoints);

    using ClientHolder =
        std::unique_ptr<sdk::AuroraClient, py::nodelete>;
    py::class_<sdk::AuroraClient, ClientHolder>(m, "AuroraClient")
        .def_static(
            "get_instance",
            []() -> sdk::AuroraClient& {
                return sdk::AuroraClient::getInstance();
            },
            py::return_value_policy::reference)
        .def("config", &sdk::AuroraClient::config)
        .def("configure", &sdk::AuroraClient::configure)
        .def(
            "set_domain_id",
            [](sdk::AuroraClient& client, const py::object& value) {
                client.setDomainId(requireUint32(value, "domain_id"));
            })
        .def("domain_id", &sdk::AuroraClient::domainId)
        .def("state", &sdk::AuroraClient::state)
        .def(
            "start",
            [](sdk::AuroraClient& client) {
                py::gil_scoped_release release;
                client.start();
            })
        .def(
            "stop",
            [](sdk::AuroraClient& client) {
                py::gil_scoped_release release;
                client.stop();
            })
        .def(
            "endpoint_match_status",
            &sdk::AuroraClient::endpointMatchStatus)
        .def(
            "is_fsm_service_matched",
            &sdk::AuroraClient::isFsmServiceMatched)
        .def(
            "wait_for_endpoints",
            [](sdk::AuroraClient& client, const py::object& timeout) {
                const auto native_timeout =
                    requireWaitDuration(timeout, "timeout");
                py::gil_scoped_release release;
                return client.waitForEndpoints(native_timeout);
            })
        .def(
            "wait_for_fsm_state",
            [](sdk::AuroraClient& client,
               const py::object& state_id,
               const py::object& timeout) {
                const auto native_state_id =
                    requireUint32(state_id, "state_id");
                const auto native_timeout =
                    requireWaitDuration(timeout, "timeout");
                py::gil_scoped_release release;
                return client.waitForFsmState(
                    native_state_id, native_timeout);
            })
        .def(
            "get_aurora_state",
            [](const sdk::AuroraClient& client) {
                sdk::AuroraState output;
                const auto result = client.getAuroraState(output);
                return py::make_tuple(result, output);
            })
        .def(
            "get_motion_state",
            [](const sdk::AuroraClient& client) {
                sdk::MotionState output;
                const auto result = client.getMotionState(output);
                return py::make_tuple(result, output);
            })
        .def(
            "get_control_group_state",
            [](const sdk::AuroraClient& client) {
                sdk::ControlGroupState output;
                const auto result = client.getControlGroupState(output);
                return py::make_tuple(result, output);
            })
        .def(
            "get_motor_configuration_state",
            [](const sdk::AuroraClient& client) {
                sdk::MotorConfigurationState output;
                const auto result = client.getMotorConfigurationState(output);
                return py::make_tuple(result, output);
            })
        .def(
            "get_estimator_data",
            [](const sdk::AuroraClient& client) {
                sdk::EstimatorData output;
                const auto result = client.getEstimatorData(output);
                return py::make_tuple(result, output);
            })
        .def(
            "get_error_codes",
            [](const sdk::AuroraClient& client) {
                sdk::ErrorCodes output;
                const auto result = client.getErrorCodes(output);
                return py::make_tuple(result, output);
            })
        .def(
            "get_base_data",
            [](const sdk::AuroraClient& client) {
                sdk::BaseData output;
                const auto result = client.getBaseData(output);
                return py::make_tuple(result, output);
            })
        .def(
            "get_contact_data",
            [](const sdk::AuroraClient& client) {
                sdk::ContactData output;
                const auto result = client.getContactData(output);
                return py::make_tuple(result, output);
            })
        .def(
            "get_stable_level",
            [](const sdk::AuroraClient& client) {
                uint8_t output = 0;
                const auto result = client.getStableLevel(output);
                return py::make_tuple(result, output);
            })
        .def(
            "get_stand_pose",
            [](const sdk::AuroraClient& client) {
                std::vector<double> output;
                const auto result = client.getStandPose(output);
                return py::make_tuple(result, output);
            })
        .def(
            "get_joint_state",
            [](const sdk::AuroraClient& client,
               const std::string& group_name,
               sdk::JointField field) {
                std::vector<double> output;
                const auto result =
                    client.getJointState(group_name, field, output);
                return py::make_tuple(result, output);
            })
        .def(
            "get_joint_command",
            [](const sdk::AuroraClient& client,
               const std::string& group_name,
               sdk::JointField field) {
                std::vector<double> output;
                const auto result =
                    client.getJointCommand(group_name, field, output);
                return py::make_tuple(result, output);
            })
        .def(
            "get_cartesian_state",
            [](const sdk::AuroraClient& client,
               const std::string& link_name) {
                sdk::CartesianStateEntry output;
                const auto result =
                    client.getCartesianState(link_name, output);
                return py::make_tuple(result, output);
            })
        .def(
            "get_cartesian_command",
            [](const sdk::AuroraClient& client,
               const std::string& link_name) {
                sdk::CartesianStateEntry output;
                const auto result =
                    client.getCartesianCommand(link_name, output);
                return py::make_tuple(result, output);
            })
        .def(
            "get_last_motion_command",
            [](const sdk::AuroraClient& client) {
                sdk::MotionCommand output;
                const auto result =
                    client.getLastMotionCommand(output);
                return py::make_tuple(result, output);
            })
        .def(
            "get_last_control_group_command",
            [](const sdk::AuroraClient& client) {
                sdk::ControlGroupCommand output;
                const auto result =
                    client.getLastControlGroupCommand(output);
                return py::make_tuple(result, output);
            })
        .def(
            "get_last_motor_configuration_command",
            [](const sdk::AuroraClient& client) {
                sdk::MotorConfigurationCommand output;
                const auto result = client.getLastMotorConfigurationCommand(output);
                return py::make_tuple(result, output);
            })
        .def(
            "get_last_sub_controller_command",
            [](const sdk::AuroraClient& client) {
                sdk::SubControllerCommand output;
                const auto result =
                    client.getLastSubControllerCommand(output);
                return py::make_tuple(result, output);
            })
        .def(
            "publish_motion_command",
            &sdk::AuroraClient::publishMotionCommand)
        .def(
            "publish_control_group_command",
            &sdk::AuroraClient::publishControlGroupCommand)
        .def(
            "publish_motor_configuration_command",
            &sdk::AuroraClient::publishMotorConfigurationCommand)
        .def("set_sub_controller", &sdk::AuroraClient::setSubController)
        .def(
            "register_lease",
            &sdk::AuroraClient::registerLease,
            py::arg("resources"), py::arg("option") = sdk::LeaseOption{})
        .def(
            "replace_lease",
            &sdk::AuroraClient::replaceLease,
            py::arg("resources"), py::arg("option") = sdk::LeaseOption{})
        .def("release_lease", &sdk::AuroraClient::releaseLease)
        .def("get_lease", &sdk::AuroraClient::getLease)
        .def(
            "on_lease_changed",
            [](sdk::AuroraClient& client, py::object callback) {
                if (callback.is_none()) {
                    client.onLeaseChanged({});
                    return;
                }
                if (!PyCallable_Check(callback.ptr())) {
                    throw sdk::InvalidArgumentError("lease callback must be callable");
                }
                client.onLeaseChanged(
                    [callback = std::move(callback)](sdk::LeaseResult result) {
                        py::gil_scoped_acquire acquire;
                        callback(std::move(result));
                    });
            })
        .def(
            "request_fsm_state",
            [](sdk::AuroraClient& client, const py::object& state_id) {
                const auto native_state_id =
                    requireUint32(state_id, "state_id");
                sdk::OperationResult result;
                {
                    py::gil_scoped_release release;
                    result = client.requestFsmState(native_state_id);
                }
                return result;
            })
        .def(
            "set_velocity",
            &sdk::AuroraClient::setVelocity,
            py::arg("vx"),
            py::arg("vy"),
            py::arg("yaw"),
            py::arg("duration"),
            py::arg("gait_type") = 0)
        .def(
            "set_stand_pose",
            &sdk::AuroraClient::setStandPose,
            py::arg("delta_height"),
            py::arg("delta_pitch"),
            py::arg("delta_yaw"),
            py::arg("duration") = 1.0)
        .def(
            "reset_unicontroller",
            &sdk::AuroraClient::resetUnicontroller);
}
