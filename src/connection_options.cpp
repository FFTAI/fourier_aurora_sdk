#include "fourier_aurora_sdk/connection_options.hpp"

#include <algorithm>

#include "fourier_aurora_sdk/errors.hpp"

namespace fourier::aurora_sdk {

NormalizedConnectionOptions normalizeConnectionOptions(const ConnectionOptions& options) {
    NormalizedConnectionOptions normalized;
    normalized.dds_mode = options.dds_mode;
    normalized.use_namespace = options.use_namespace;
    normalized.service_timeout_ms = options.service_timeout_ms;

    if (options.namespace_name.has_value()) {
        normalized.namespace_name = *options.namespace_name;
        normalized.has_namespace_override = true;
    }
    if (options.is_ros_compatible.has_value()) {
        normalized.is_ros_compatible = *options.is_ros_compatible;
        normalized.has_ros_compatible_override = true;
    }
    if (options.use_discovery_server.has_value()) {
        normalized.use_discovery_server = *options.use_discovery_server;
        normalized.has_discovery_server_override = true;
    }
    return normalized;
}

void validateUserId(const std::string& user_id) {
    if (user_id.empty()) {
        throw InvalidArgumentError("user_id must be non-empty");
    }
    if (user_id.size() > 128) {
        throw InvalidArgumentError("user_id must be 128 bytes or less");
    }
    if (std::find(user_id.begin(), user_id.end(), '\0') != user_id.end()) {
        throw InvalidArgumentError("user_id must not contain embedded NUL bytes");
    }
}

}  // namespace fourier::aurora_sdk
