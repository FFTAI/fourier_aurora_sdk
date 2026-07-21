#include "internal/connection_options.hpp"

#include <algorithm>
#include <array>

#include "fourier_aurora_sdk/errors.hpp"

namespace fourier::aurora_sdk::detail {

namespace {

constexpr std::array<Endpoint, 9> kAllEndpoints{
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

void validateBoundedString(const std::string& value, const char* field_name) {
    if (value.empty()) {
        throw InvalidArgumentError(std::string(field_name) + " must be non-empty");
    }
    if (value.size() > 128) {
        throw InvalidArgumentError(std::string(field_name) + " must be 128 bytes or less");
    }
    if (std::find(value.begin(), value.end(), '\0') != value.end()) {
        throw InvalidArgumentError(
            std::string(field_name) + " must not contain embedded NUL bytes");
    }
}

bool isDeclaredEndpoint(Endpoint endpoint) {
    return std::find(kAllEndpoints.begin(), kAllEndpoints.end(), endpoint) !=
           kAllEndpoints.end();
}

std::vector<Endpoint> normalizeEndpoints(const std::vector<Endpoint>& endpoints) {
    if (endpoints.empty()) {
        throw InvalidArgumentError("enabled_endpoints must not be empty");
    }
    for (const auto endpoint : endpoints) {
        if (!isDeclaredEndpoint(endpoint)) {
            throw InvalidArgumentError("enabled_endpoints contains an invalid Endpoint value");
        }
    }

    std::vector<Endpoint> normalized;
    normalized.reserve(kAllEndpoints.size());
    for (const auto endpoint : kAllEndpoints) {
        if (std::find(endpoints.begin(), endpoints.end(), endpoint) != endpoints.end()) {
            normalized.push_back(endpoint);
        }
    }
    return normalized;
}

}  // namespace

std::size_t endpointIndex(Endpoint endpoint) {
    const auto it = std::find(kAllEndpoints.begin(), kAllEndpoints.end(), endpoint);
    if (it == kAllEndpoints.end()) {
        throw InvalidArgumentError("invalid Endpoint value");
    }
    return static_cast<std::size_t>(std::distance(kAllEndpoints.begin(), it));
}

bool NormalizedConnectionOptions::endpointEnabled(Endpoint endpoint) const {
    return enabled[endpointIndex(endpoint)];
}

NormalizedConnectionOptions normalizeConnectionOptions(
    const ConnectionOptions& options) {
    if (options.dds_mode > 3) {
        throw InvalidArgumentError("dds_mode must be one of 0, 1, 2, or 3");
    }

    NormalizedConnectionOptions normalized;
    normalized.public_options = options;
    normalized.public_options.enabled_endpoints =
        normalizeEndpoints(options.enabled_endpoints);
    for (const auto endpoint : normalized.public_options.enabled_endpoints) {
        normalized.enabled[endpointIndex(endpoint)] = true;
    }

    if (options.namespace_name.has_value()) {
        validateBoundedString(*options.namespace_name, "namespace_name");
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
    validateBoundedString(user_id, "user_id");
}

}  // namespace fourier::aurora_sdk::detail
