#pragma once

#include <cstdint>
#include <optional>
#include <string>
#include <vector>

#include "fourier_aurora_sdk/types/common.hpp"

namespace fourier::aurora_sdk {

struct ConnectionOptions {
    std::optional<std::string> namespace_name;
    uint8_t dds_mode = 0;
    std::optional<bool> is_ros_compatible;
    bool use_namespace = true;
    std::optional<bool> use_discovery_server;
    uint32_t service_timeout_ms = 1000;
    std::vector<Endpoint> enabled_endpoints{
        Endpoint::FsmService,
        Endpoint::MotionCommandPublisher,
        Endpoint::ControlGroupCommandPublisher,
        Endpoint::SubControllerCommandPublisher,
        Endpoint::AuroraStateSubscriber,
        Endpoint::MotionStateSubscriber,
        Endpoint::ControlGroupStateSubscriber,
        Endpoint::EstimatorDataSubscriber,
        Endpoint::ErrorCodesSubscriber,
    };

    bool operator==(const ConnectionOptions& other) const {
        return namespace_name == other.namespace_name && dds_mode == other.dds_mode &&
               is_ros_compatible == other.is_ros_compatible &&
               use_namespace == other.use_namespace &&
               use_discovery_server == other.use_discovery_server &&
               service_timeout_ms == other.service_timeout_ms &&
               enabled_endpoints == other.enabled_endpoints;
    }
};

}  // namespace fourier::aurora_sdk
