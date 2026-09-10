#include "internal/connection_options.hpp"

#include <algorithm>
#include <array>

#include "fourier_aurora_sdk/errors.hpp"

namespace fourier::aurora_sdk::detail {

namespace {

constexpr std::array<Endpoint, 12> kAllEndpoints{
    Endpoint::FsmService,
    Endpoint::ManageLeaseService,
    Endpoint::MotionCommandPublisher,
    Endpoint::ControlGroupCommandPublisher,
    Endpoint::MotorConfigurationCommandPublisher,
    Endpoint::SubControllerService,
    Endpoint::AuroraStateSubscriber,
    Endpoint::MotionStateSubscriber,
    Endpoint::ControlGroupStateSubscriber,
    Endpoint::MotorConfigurationStateSubscriber,
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

void validateTopicPrefix(const std::string& value) {
    if (value.size() > 128) {
        throw InvalidArgumentError("topic_prefix must be 128 bytes or less");
    }
    if (std::any_of(value.begin(), value.end(),
                    [](unsigned char c) { return c < 0x20 || c == 0x7f; })) {
        throw InvalidArgumentError(
            "topic_prefix must not contain control characters");
    }
}

bool endpointEnabled(const std::vector<Endpoint>& endpoints, Endpoint endpoint) {
    return std::find(endpoints.begin(), endpoints.end(), endpoint) != endpoints.end();
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
    validateTopicPrefix(options.topic_prefix);
    normalized.public_options = options;
    normalized.public_options.enabled_endpoints =
        normalizeEndpoints(options.enabled_endpoints);
    for (const auto endpoint : normalized.public_options.enabled_endpoints) {
        normalized.enabled[endpointIndex(endpoint)] = true;
    }

    const auto& endpoints = normalized.public_options.enabled_endpoints;
    const bool has_manage = endpointEnabled(endpoints, Endpoint::ManageLeaseService);
    const bool has_protected_publisher =
        endpointEnabled(endpoints, Endpoint::MotionCommandPublisher) ||
        endpointEnabled(endpoints, Endpoint::ControlGroupCommandPublisher) ||
        endpointEnabled(endpoints, Endpoint::MotorConfigurationCommandPublisher);
    if (has_protected_publisher && !has_manage) {
        throw InvalidArgumentError(
            "protected command publishers require ManageLeaseService");
    }
    if (options.source_name.empty() || options.source_name.size() > 64 ||
        !std::all_of(options.source_name.begin(), options.source_name.end(),
                     [](unsigned char c) {
                         return (c >= 'A' && c <= 'Z') ||
                                (c >= 'a' && c <= 'z') ||
                                (c >= '0' && c <= '9') || c == '.' ||
                                c == '_' || c == '-';
                     })) {
        throw InvalidArgumentError(
            "source_name must match [A-Za-z0-9._-]{1,64}");
    }
    if (options.allow_emergency_lease_priority && !has_manage) {
        throw InvalidArgumentError(
            "Emergency lease opt-in requires ManageLeaseService");
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

}  // namespace fourier::aurora_sdk::detail
