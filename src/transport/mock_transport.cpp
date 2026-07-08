#include "fourier_aurora_sdk/transport/mock_transport.hpp"

namespace fourier::aurora_sdk {

void MockTransport::setAuroraStateHandler(AuroraStateHandler handler) {
    aurora_state_handler_ = std::move(handler);
}

void MockTransport::setMotionStateHandler(MotionStateHandler handler) {
    motion_state_handler_ = std::move(handler);
}

void MockTransport::setControlGroupStateHandler(ControlGroupStateHandler handler) {
    control_group_state_handler_ = std::move(handler);
}

void MockTransport::setEstimatorDataHandler(EstimatorDataHandler handler) {
    estimator_data_handler_ = std::move(handler);
}

void MockTransport::setErrorCodesHandler(ErrorCodesHandler handler) {
    error_codes_handler_ = std::move(handler);
}

void MockTransport::publishMotionCommand(const MotionCommand& command) {
    motion_commands.push_back(command);
}

void MockTransport::publishControlGroupCommand(const ControlGroupCommand& command) {
    control_group_commands.push_back(command);
}

void MockTransport::publishSubControllerCommand(const SubControllerCommand& command) {
    sub_controller_commands.push_back(command);
}

ServiceCallResult MockTransport::requestFsmState(uint32_t state_id, uint32_t /*timeout_ms*/) {
    fsm_requests.push_back(state_id);
    return next_service_result;
}

void MockTransport::injectAuroraState(const AuroraState& state) {
    if (aurora_state_handler_) {
        aurora_state_handler_(state);
    }
}

void MockTransport::injectMotionState(const MotionState& state) {
    if (motion_state_handler_) {
        motion_state_handler_(state);
    }
}

void MockTransport::injectControlGroupState(const ControlGroupState& state) {
    if (control_group_state_handler_) {
        control_group_state_handler_(state);
    }
}

void MockTransport::injectEstimatorData(const EstimatorData& data) {
    if (estimator_data_handler_) {
        estimator_data_handler_(data);
    }
}

void MockTransport::injectErrorCodes(const ErrorCodes& codes) {
    if (error_codes_handler_) {
        error_codes_handler_(codes);
    }
}

}  // namespace fourier::aurora_sdk
