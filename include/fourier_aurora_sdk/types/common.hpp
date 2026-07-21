#pragma once

#include <chrono>
#include <cstdint>
#include <string>
#include <string_view>

namespace fourier::aurora_sdk {

enum class ClientState : uint8_t {
    Stopped,
    Starting,
    Running,
    Stopping,
};

enum class Endpoint : uint8_t {
    FsmService,
    MotionCommandPublisher,
    ControlGroupCommandPublisher,
    SubControllerCommandPublisher,
    AuroraStateSubscriber,
    MotionStateSubscriber,
    ControlGroupStateSubscriber,
    EstimatorDataSubscriber,
    ErrorCodesSubscriber,
};

struct EndpointStatus {
    bool enabled = false;
    bool matched = false;
};

struct EndpointMatchStatus {
    EndpointStatus fsm_service;
    EndpointStatus motion_command_publisher;
    EndpointStatus control_group_command_publisher;
    EndpointStatus sub_controller_command_publisher;
    EndpointStatus aurora_state_subscriber;
    EndpointStatus motion_state_subscriber;
    EndpointStatus control_group_state_subscriber;
    EndpointStatus estimator_data_subscriber;
    EndpointStatus error_codes_subscriber;
};

namespace result_codes {

inline constexpr uint32_t SUCCESS = 0;
inline constexpr uint32_t CLIENT_ERROR = 100;
inline constexpr uint32_t CLIENT_NOT_RUNNING = 101;
inline constexpr uint32_t ENDPOINT_DISABLED = 102;
inline constexpr uint32_t ENDPOINT_UNMATCHED = 103;
inline constexpr uint32_t PUBLISH_FAILED = 104;
inline constexpr uint32_t SERVICE_TIMEOUT = 105;
inline constexpr uint32_t TRANSPORT_ERROR = 106;
inline constexpr uint32_t INVALID_AURORA_RESPONSE = 107;
inline constexpr uint32_t STATE_NOT_RECEIVED = 108;

}  // namespace result_codes

[[nodiscard]] inline constexpr std::string_view resultCodeName(uint32_t code) noexcept {
    switch (code) {
        case result_codes::SUCCESS:
            return "SUCCESS";
        case result_codes::CLIENT_ERROR:
            return "CLIENT_ERROR";
        case result_codes::CLIENT_NOT_RUNNING:
            return "CLIENT_NOT_RUNNING";
        case result_codes::ENDPOINT_DISABLED:
            return "ENDPOINT_DISABLED";
        case result_codes::ENDPOINT_UNMATCHED:
            return "ENDPOINT_UNMATCHED";
        case result_codes::PUBLISH_FAILED:
            return "PUBLISH_FAILED";
        case result_codes::SERVICE_TIMEOUT:
            return "SERVICE_TIMEOUT";
        case result_codes::TRANSPORT_ERROR:
            return "TRANSPORT_ERROR";
        case result_codes::INVALID_AURORA_RESPONSE:
            return "INVALID_AURORA_RESPONSE";
        case result_codes::STATE_NOT_RECEIVED:
            return "STATE_NOT_RECEIVED";
        default:
            return "UNKNOWN_RESULT_CODE";
    }
}

[[nodiscard]] inline constexpr std::string_view defaultResultMessage(uint32_t code) noexcept {
    switch (code) {
        case result_codes::SUCCESS:
            return "success";
        case result_codes::CLIENT_ERROR:
            return "client operation failed";
        case result_codes::CLIENT_NOT_RUNNING:
            return "client is not running";
        case result_codes::ENDPOINT_DISABLED:
            return "required endpoint is disabled";
        case result_codes::ENDPOINT_UNMATCHED:
            return "required endpoint is not matched";
        case result_codes::PUBLISH_FAILED:
            return "DDS rejected the publish";
        case result_codes::SERVICE_TIMEOUT:
            return "service request timed out";
        case result_codes::TRANSPORT_ERROR:
            return "transport operation failed";
        case result_codes::INVALID_AURORA_RESPONSE:
            return "Aurora returned a malformed response";
        case result_codes::STATE_NOT_RECEIVED:
            return "state has not been received";
        default:
            return "unknown result code";
    }
}

struct OperationResult {
    uint32_t code = result_codes::CLIENT_ERROR;
    std::string message{defaultResultMessage(result_codes::CLIENT_ERROR)};

    [[nodiscard]] bool success() const noexcept {
        return code == result_codes::SUCCESS;
    }
};

struct MessageHeader {
    std::chrono::system_clock::time_point send_time{};
    std::string frame_id;
    std::chrono::system_clock::time_point received_time{};
};

}  // namespace fourier::aurora_sdk
