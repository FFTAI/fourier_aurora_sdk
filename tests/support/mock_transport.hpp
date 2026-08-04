#pragma once

#include <chrono>
#include <condition_variable>
#include <cstddef>
#include <cstdint>
#include <memory>
#include <mutex>
#include <thread>
#include <vector>

#include "internal/transport.hpp"

namespace fourier::aurora_sdk::testing {

struct MockSession {
    mutable std::mutex mutex;
    std::condition_variable changed;
    uint32_t domain_id = 0;
    ConnectionOptions options;
    detail::TransportHandlers handlers;
    detail::TransportTerminalObserver terminal_observer;
    EndpointMatchStatus match_status;

    OperationResult motion_publish_result{
        result_codes::SUCCESS,
        std::string(defaultResultMessage(result_codes::SUCCESS))};
    OperationResult control_group_publish_result{
        result_codes::SUCCESS,
        std::string(defaultResultMessage(result_codes::SUCCESS))};
    OperationResult motor_configuration_publish_result{
        result_codes::SUCCESS,
        std::string(defaultResultMessage(result_codes::SUCCESS))};
    OperationResult sub_controller_result{
        result_codes::SUCCESS,
        std::string(defaultResultMessage(result_codes::SUCCESS))};
    OperationResult fsm_result{
        result_codes::SUCCESS,
        "success"};
    detail::LeaseManagementResponse lease_response{
        {result_codes::SUCCESS, "success"}, 0, 1, 2.0, "lease accepted"};

    bool stopped = false;
    bool block_stop = false;
    bool release_stop = true;
    bool throw_stop = false;
    bool throw_nonstandard_stop = false;
    bool block_request = false;
    bool release_request = true;
    bool fatal_request = false;
    bool fatal_publish = false;
    std::size_t stop_calls = 0;
    std::size_t request_calls = 0;
    std::thread::id stop_thread;
    std::vector<uint32_t> fsm_requests;
    std::vector<MotionCommand> motion_commands;
    std::vector<detail::LeaseToken> motion_tokens;
    std::vector<ControlGroupCommand> control_group_commands;
    std::vector<detail::LeaseToken> control_group_tokens;
    std::vector<MotorConfigurationCommand> motor_configuration_commands;
    std::vector<detail::LeaseToken> motor_configuration_tokens;
    std::vector<SubControllerCommand> sub_controller_commands;
    std::vector<detail::LeaseManagementRequest> lease_requests;

    void setMatched(Endpoint endpoint, bool matched);
    void setAllEnabledMatched(bool matched);
    void injectAuroraState(AuroraState state);
    void injectMotionState(MotionState state);
    void injectControlGroupState(ControlGroupState state);
    void injectEstimatorData(EstimatorData data);
    void injectErrorCodes(ErrorCodes codes);
};

struct MockFactoryControl {
    mutable std::mutex mutex;
    std::condition_variable changed;
    bool block_create = false;
    bool release_create = true;
    bool fail_create = false;
    bool throw_nonstandard_create = false;
    bool match_on_create = true;
    bool inject_motion_during_create = false;
    MotionState create_motion_sample;
    std::size_t create_calls = 0;
    std::shared_ptr<MockSession> latest_session;

    void reset();
    [[nodiscard]] std::shared_ptr<MockSession> latest() const;
    [[nodiscard]] bool waitForCreateCalls(
        std::size_t count,
        std::chrono::milliseconds timeout);
};

[[nodiscard]] MockFactoryControl& mockFactoryControl();
void installMockTransportFactory();
void resetMockTransportFixture();

}  // namespace fourier::aurora_sdk::testing
