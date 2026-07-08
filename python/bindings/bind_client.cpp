#include <pybind11/pybind11.h>
#include <pybind11/stl.h>

#include "fourier_aurora_sdk/aurora_client.hpp"

namespace py = pybind11;
namespace sdk = fourier::aurora_sdk;

void bind_client(py::module_& m) {
    py::class_<sdk::ConnectionOptions>(m, "ConnectionOptions")
        .def(py::init<>())
        .def_readwrite("namespace_name", &sdk::ConnectionOptions::namespace_name)
        .def_readwrite("dds_mode", &sdk::ConnectionOptions::dds_mode)
        .def_readwrite("is_ros_compatible", &sdk::ConnectionOptions::is_ros_compatible)
        .def_readwrite("use_namespace", &sdk::ConnectionOptions::use_namespace)
        .def_readwrite("use_discovery_server", &sdk::ConnectionOptions::use_discovery_server)
        .def_readwrite("service_timeout_ms", &sdk::ConnectionOptions::service_timeout_ms);

    py::class_<sdk::AuroraClient>(m, "AuroraClient")
        .def_static("get_instance", &sdk::AuroraClient::getInstance,
            py::arg("user_id"), py::arg("domain_id") = 123, py::arg("options") = sdk::ConnectionOptions{},
            py::return_value_policy::reference)
        .def("close", &sdk::AuroraClient::close)
        .def("getAuroraState", &sdk::AuroraClient::getAuroraState)
        .def("getMotionState", &sdk::AuroraClient::getMotionState)
        .def("getControlGroupState", &sdk::AuroraClient::getControlGroupState)
        .def("getEstimatorData", &sdk::AuroraClient::getEstimatorData)
        .def("getErrorCodes", &sdk::AuroraClient::getErrorCodes)
        .def("getBaseData", &sdk::AuroraClient::getBaseData)
        .def("getContactData", &sdk::AuroraClient::getContactData)
        .def("getStableLevel", &sdk::AuroraClient::getStableLevel)
        .def("getStandPose", &sdk::AuroraClient::getStandPose)
        .def("getJointState", &sdk::AuroraClient::getJointState)
        .def("getJointCommand", &sdk::AuroraClient::getJointCommand)
        .def("getCartesianState", &sdk::AuroraClient::getCartesianState)
        .def("getCartesianCommand", &sdk::AuroraClient::getCartesianCommand)
        .def("publishMotionCommand", &sdk::AuroraClient::publishMotionCommand)
        .def("publishControlGroupCommand", &sdk::AuroraClient::publishControlGroupCommand)
        .def("publishSubControllerCommand", &sdk::AuroraClient::publishSubControllerCommand)
        .def("requestFsmState", &sdk::AuroraClient::requestFsmState)
        .def("isFsmServiceMatched", &sdk::AuroraClient::isFsmServiceMatched)
        .def("setVelocity", &sdk::AuroraClient::setVelocity,
            py::arg("vx"), py::arg("vy"), py::arg("yaw"), py::arg("duration"), py::arg("gait_type") = 0)
        .def("setStandPose", &sdk::AuroraClient::setStandPose,
            py::arg("delta_height"), py::arg("delta_pitch"), py::arg("delta_yaw"), py::arg("duration") = 1.0)
        .def("resetUnicontroller", &sdk::AuroraClient::resetUnicontroller);
}
