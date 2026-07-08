#pragma once

#include <cstdint>
#include <optional>
#include <string>

namespace fourier::aurora_sdk {

struct ConnectionOptions {
    std::optional<std::string> namespace_name;
    uint8_t dds_mode = 0;
    std::optional<bool> is_ros_compatible;
    bool use_namespace = true;
    std::optional<bool> use_discovery_server;
    uint32_t service_timeout_ms = 1000;

    bool operator==(const ConnectionOptions& other) const {
        return namespace_name == other.namespace_name && dds_mode == other.dds_mode &&
               is_ros_compatible == other.is_ros_compatible &&
               use_namespace == other.use_namespace &&
               use_discovery_server == other.use_discovery_server &&
               service_timeout_ms == other.service_timeout_ms;
    }
};

struct NormalizedConnectionOptions {
    std::string namespace_name;
    bool has_namespace_override = false;
    uint8_t dds_mode = 0;
    bool is_ros_compatible = false;
    bool has_ros_compatible_override = false;
    bool use_namespace = true;
    bool use_discovery_server = false;
    bool has_discovery_server_override = false;
    uint32_t service_timeout_ms = 1000;

    bool operator==(const NormalizedConnectionOptions& other) const {
        return namespace_name == other.namespace_name &&
               has_namespace_override == other.has_namespace_override &&
               dds_mode == other.dds_mode &&
               is_ros_compatible == other.is_ros_compatible &&
               has_ros_compatible_override == other.has_ros_compatible_override &&
               use_namespace == other.use_namespace &&
               use_discovery_server == other.use_discovery_server &&
               has_discovery_server_override == other.has_discovery_server_override &&
               service_timeout_ms == other.service_timeout_ms;
    }
};

NormalizedConnectionOptions normalizeConnectionOptions(const ConnectionOptions& options);
void validateUserId(const std::string& user_id);

}  // namespace fourier::aurora_sdk
