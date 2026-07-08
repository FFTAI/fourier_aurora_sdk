#pragma once

#include <optional>
#include <vector>

#include "fourier_aurora_sdk/transport/transport.hpp"

namespace fourier::aurora_sdk {

class MockTransport : public ITransport {
public:
    void setAuroraStateHandler(AuroraStateHandler handler) override;
    void setMotionStateHandler(MotionStateHandler handler) override;
    void setControlGroupStateHandler(ControlGroupStateHandler handler) override;
    void setEstimatorDataHandler(EstimatorDataHandler handler) override;
    void setErrorCodesHandler(ErrorCodesHandler handler) override;

    void publishMotionCommand(const MotionCommand& command) override;
    void publishControlGroupCommand(const ControlGroupCommand& command) override;
    void publishSubControllerCommand(const SubControllerCommand& command) override;
    ServiceCallResult requestFsmState(uint32_t state_id, uint32_t timeout_ms) override;

    bool isMotionCommandPublisherMatched() const override { return motion_command_publisher_matched; }
    bool isControlGroupCommandPublisherMatched() const override { return control_group_command_publisher_matched; }
    bool isSubControllerCommandPublisherMatched() const override { return sub_controller_command_publisher_matched; }
    bool isAuroraStateSubscriberMatched() const override { return aurora_state_subscriber_matched; }
    bool isMotionStateSubscriberMatched() const override { return motion_state_subscriber_matched; }
    bool isControlGroupStateSubscriberMatched() const override { return control_group_state_subscriber_matched; }
    bool isEstimatorDataSubscriberMatched() const override { return estimator_data_subscriber_matched; }
    bool isErrorCodesSubscriberMatched() const override { return error_codes_subscriber_matched; }

    void close() override { closed = true; }

    void injectAuroraState(const AuroraState& state);
    void injectMotionState(const MotionState& state);
    void injectControlGroupState(const ControlGroupState& state);
    void injectEstimatorData(const EstimatorData& data);
    void injectErrorCodes(const ErrorCodes& codes);

    bool closed = false;
    bool motion_command_publisher_matched = true;
    bool control_group_command_publisher_matched = true;
    bool sub_controller_command_publisher_matched = true;
    bool aurora_state_subscriber_matched = true;
    bool motion_state_subscriber_matched = true;
    bool control_group_state_subscriber_matched = true;
    bool estimator_data_subscriber_matched = true;
    bool error_codes_subscriber_matched = true;

    std::vector<MotionCommand> motion_commands;
    std::vector<ControlGroupCommand> control_group_commands;
    std::vector<SubControllerCommand> sub_controller_commands;
    std::vector<uint32_t> fsm_requests;
    ServiceCallResult next_service_result;

private:
    AuroraStateHandler aurora_state_handler_;
    MotionStateHandler motion_state_handler_;
    ControlGroupStateHandler control_group_state_handler_;
    EstimatorDataHandler estimator_data_handler_;
    ErrorCodesHandler error_codes_handler_;
};

}  // namespace fourier::aurora_sdk
