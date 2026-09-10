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
    ManageLeaseService,
    MotionCommandPublisher,
    ControlGroupCommandPublisher,
    MotorConfigurationCommandPublisher,
    SubControllerService,
    AuroraStateSubscriber,
    MotionStateSubscriber,
    ControlGroupStateSubscriber,
    MotorConfigurationStateSubscriber,
    EstimatorDataSubscriber,
    ErrorCodesSubscriber,
};

struct EndpointStatus {
    bool enabled = false;
    bool matched = false;
};

struct EndpointMatchStatus {
    EndpointStatus fsm_service;
    EndpointStatus manage_lease_service;
    EndpointStatus motion_command_publisher;
    EndpointStatus control_group_command_publisher;
    EndpointStatus motor_configuration_command_publisher;
    EndpointStatus sub_controller_service;
    EndpointStatus aurora_state_subscriber;
    EndpointStatus motion_state_subscriber;
    EndpointStatus control_group_state_subscriber;
    EndpointStatus motor_configuration_state_subscriber;
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

inline constexpr uint32_t FSM_INVALID_REQUEST = 200;
inline constexpr uint32_t FSM_STATE_NOT_FOUND = 201;
inline constexpr uint32_t FSM_ENTER_CHECK_FAILED = 202;
inline constexpr uint32_t FSM_SWITCH_BLOCKER_UNHEALTHY = 203;
inline constexpr uint32_t FSM_SWITCH_BUSY = 204;
inline constexpr uint32_t FSM_SHUTTING_DOWN = 205;
inline constexpr uint32_t FSM_STATE_MANAGER_UNAVAILABLE = 206;
inline constexpr uint32_t FSM_INTERNAL_ERROR = 207;

inline constexpr uint32_t LEASE_INVALID_REQUEST = 300;
inline constexpr uint32_t LEASE_UNKNOWN_RESOURCE = 301;
inline constexpr uint32_t LEASE_PRIORITY_NOT_ALLOWED = 302;
inline constexpr uint32_t LEASE_RESOURCE_BUSY = 303;
inline constexpr uint32_t LEASE_NOT_FOUND = 304;
inline constexpr uint32_t LEASE_EXPIRED = 305;
inline constexpr uint32_t LEASE_OPERATION_ID_CONFLICT = 306;
inline constexpr uint32_t LEASE_AUTHORITY_ERROR = 307;
inline constexpr uint32_t LEASE_SOURCE_ALREADY_ACTIVE = 308;
inline constexpr uint32_t LEASE_AUTHORITY_BUSY = 309;
inline constexpr uint32_t LEASE_NOT_ACTIVE = 310;
inline constexpr uint32_t LEASE_RESOURCE_NOT_COVERED = 311;
inline constexpr uint32_t LEASE_OPERATION_IN_PROGRESS = 312;
inline constexpr uint32_t LEASE_LOST = 313;
inline constexpr uint32_t LEASE_OPERATION_TIMEOUT = 314;
inline constexpr uint32_t SUB_CONTROLLER_NO_ACTIVE_UNICONTROLLER = 320;
inline constexpr uint32_t SUB_CONTROLLER_STATE_SWITCH_BUSY = 321;
inline constexpr uint32_t SUB_CONTROLLER_INVALID_REQUEST = 322;
inline constexpr uint32_t SUB_CONTROLLER_INTERNAL_ERROR = 323;

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
        case result_codes::FSM_INVALID_REQUEST: return "FSM_INVALID_REQUEST";
        case result_codes::FSM_STATE_NOT_FOUND: return "FSM_STATE_NOT_FOUND";
        case result_codes::FSM_ENTER_CHECK_FAILED: return "FSM_ENTER_CHECK_FAILED";
        case result_codes::FSM_SWITCH_BLOCKER_UNHEALTHY: return "FSM_SWITCH_BLOCKER_UNHEALTHY";
        case result_codes::FSM_SWITCH_BUSY: return "FSM_SWITCH_BUSY";
        case result_codes::FSM_SHUTTING_DOWN: return "FSM_SHUTTING_DOWN";
        case result_codes::FSM_STATE_MANAGER_UNAVAILABLE: return "FSM_STATE_MANAGER_UNAVAILABLE";
        case result_codes::FSM_INTERNAL_ERROR: return "FSM_INTERNAL_ERROR";
        case result_codes::LEASE_INVALID_REQUEST: return "LEASE_INVALID_REQUEST";
        case result_codes::LEASE_UNKNOWN_RESOURCE: return "LEASE_UNKNOWN_RESOURCE";
        case result_codes::LEASE_PRIORITY_NOT_ALLOWED: return "LEASE_PRIORITY_NOT_ALLOWED";
        case result_codes::LEASE_RESOURCE_BUSY: return "LEASE_RESOURCE_BUSY";
        case result_codes::LEASE_NOT_FOUND: return "LEASE_NOT_FOUND";
        case result_codes::LEASE_EXPIRED: return "LEASE_EXPIRED";
        case result_codes::LEASE_OPERATION_ID_CONFLICT: return "LEASE_OPERATION_ID_CONFLICT";
        case result_codes::LEASE_AUTHORITY_ERROR: return "LEASE_AUTHORITY_ERROR";
        case result_codes::LEASE_SOURCE_ALREADY_ACTIVE: return "LEASE_SOURCE_ALREADY_ACTIVE";
        case result_codes::LEASE_AUTHORITY_BUSY: return "LEASE_AUTHORITY_BUSY";
        case result_codes::LEASE_NOT_ACTIVE: return "LEASE_NOT_ACTIVE";
        case result_codes::LEASE_RESOURCE_NOT_COVERED: return "LEASE_RESOURCE_NOT_COVERED";
        case result_codes::LEASE_OPERATION_IN_PROGRESS: return "LEASE_OPERATION_IN_PROGRESS";
        case result_codes::LEASE_LOST: return "LEASE_LOST";
        case result_codes::LEASE_OPERATION_TIMEOUT: return "LEASE_OPERATION_TIMEOUT";
        case result_codes::SUB_CONTROLLER_NO_ACTIVE_UNICONTROLLER: return "SUB_CONTROLLER_NO_ACTIVE_UNICONTROLLER";
        case result_codes::SUB_CONTROLLER_STATE_SWITCH_BUSY: return "SUB_CONTROLLER_STATE_SWITCH_BUSY";
        case result_codes::SUB_CONTROLLER_INVALID_REQUEST: return "SUB_CONTROLLER_INVALID_REQUEST";
        case result_codes::SUB_CONTROLLER_INTERNAL_ERROR: return "SUB_CONTROLLER_INTERNAL_ERROR";
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
        case result_codes::FSM_INVALID_REQUEST: return "FSM switch request is invalid";
        case result_codes::FSM_STATE_NOT_FOUND: return "FSM state was not found";
        case result_codes::FSM_ENTER_CHECK_FAILED: return "FSM enter check failed";
        case result_codes::FSM_SWITCH_BLOCKER_UNHEALTHY: return "FSM switch blocker is unhealthy";
        case result_codes::FSM_SWITCH_BUSY: return "FSM state switch is busy";
        case result_codes::FSM_SHUTTING_DOWN: return "Aurora is shutting down";
        case result_codes::FSM_STATE_MANAGER_UNAVAILABLE: return "FSM state manager is unavailable";
        case result_codes::FSM_INTERNAL_ERROR: return "FSM state switch failed";
        case result_codes::LEASE_INVALID_REQUEST: return "lease request is invalid";
        case result_codes::LEASE_UNKNOWN_RESOURCE: return "lease resource is unknown";
        case result_codes::LEASE_PRIORITY_NOT_ALLOWED: return "lease priority is not allowed";
        case result_codes::LEASE_RESOURCE_BUSY: return "lease resource is busy";
        case result_codes::LEASE_NOT_FOUND: return "lease was not found";
        case result_codes::LEASE_EXPIRED: return "lease has expired";
        case result_codes::LEASE_OPERATION_ID_CONFLICT: return "lease operation id conflicts";
        case result_codes::LEASE_AUTHORITY_ERROR: return "lease authority failed";
        case result_codes::LEASE_SOURCE_ALREADY_ACTIVE: return "source already has an active lease";
        case result_codes::LEASE_AUTHORITY_BUSY: return "lease authority is busy";
        case result_codes::LEASE_NOT_ACTIVE: return "no active lease";
        case result_codes::LEASE_RESOURCE_NOT_COVERED: return "active lease does not cover command resources";
        case result_codes::LEASE_OPERATION_IN_PROGRESS: return "lease operation is in progress";
        case result_codes::LEASE_LOST: return "lease was lost";
        case result_codes::LEASE_OPERATION_TIMEOUT: return "lease operation timed out";
        case result_codes::SUB_CONTROLLER_NO_ACTIVE_UNICONTROLLER: return "no active universal controller";
        case result_codes::SUB_CONTROLLER_STATE_SWITCH_BUSY: return "Aurora state switch is busy";
        case result_codes::SUB_CONTROLLER_INVALID_REQUEST: return "sub-controller request is invalid";
        case result_codes::SUB_CONTROLLER_INTERNAL_ERROR: return "sub-controller service failed";
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
