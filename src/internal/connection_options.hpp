#pragma once

#include <array>
#include <cstddef>
#include <string>

#include "fourier_aurora_sdk/connection_options.hpp"

namespace fourier::aurora_sdk::detail {

inline constexpr std::size_t kEndpointCount = 9;

struct NormalizedConnectionOptions {
    ConnectionOptions public_options;
    std::string namespace_name;
    bool has_namespace_override = false;
    bool is_ros_compatible = false;
    bool has_ros_compatible_override = false;
    bool use_discovery_server = false;
    bool has_discovery_server_override = false;
    std::array<bool, kEndpointCount> enabled{};

    [[nodiscard]] bool endpointEnabled(Endpoint endpoint) const;
};

[[nodiscard]] std::size_t endpointIndex(Endpoint endpoint);
[[nodiscard]] NormalizedConnectionOptions normalizeConnectionOptions(
    const ConnectionOptions& options);
void validateUserId(const std::string& user_id);

}  // namespace fourier::aurora_sdk::detail
