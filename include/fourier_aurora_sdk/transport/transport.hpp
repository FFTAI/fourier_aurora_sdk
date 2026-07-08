#pragma once

#include <functional>

#include "fourier_aurora_sdk/types.hpp"

namespace fourier::aurora_sdk {

using AuroraStateHandler = std::function<void(AuroraState)>;
using MotionStateHandler = std::function<void(MotionState)>;
using ControlGroupStateHandler = std::function<void(ControlGroupState)>;
using EstimatorDataHandler = std::function<void(EstimatorData)>;
using ErrorCodesHandler = std::function<void(ErrorCodes)>;

class ITransport {
public:
    virtual ~ITransport() = default;

    virtual void setAuroraStateHandler(AuroraStateHandler handler) = 0;
    virtual void setMotionStateHandler(MotionStateHandler handler) = 0;
    virtual void setControlGroupStateHandler(ControlGroupStateHandler handler) = 0;
    virtual void setEstimatorDataHandler(EstimatorDataHandler handler) = 0;
    virtual void setErrorCodesHandler(ErrorCodesHandler handler) = 0;

    virtual void publishMotionCommand(const MotionCommand& command) = 0;
    virtual void publishControlGroupCommand(const ControlGroupCommand& command) = 0;
    virtual void publishSubControllerCommand(const SubControllerCommand& command) = 0;
    virtual ServiceCallResult requestFsmState(uint32_t state_id, uint32_t timeout_ms) = 0;

    virtual bool isMotionCommandPublisherMatched() const = 0;
    virtual bool isControlGroupCommandPublisherMatched() const = 0;
    virtual bool isSubControllerCommandPublisherMatched() const = 0;
    virtual bool isAuroraStateSubscriberMatched() const = 0;
    virtual bool isMotionStateSubscriberMatched() const = 0;
    virtual bool isControlGroupStateSubscriberMatched() const = 0;
    virtual bool isEstimatorDataSubscriberMatched() const = 0;
    virtual bool isErrorCodesSubscriberMatched() const = 0;

    virtual void close() = 0;
};

}  // namespace fourier::aurora_sdk
