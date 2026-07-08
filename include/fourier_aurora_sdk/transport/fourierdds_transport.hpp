#pragma once

#include <memory>

#include "fourier_aurora_sdk/connection_options.hpp"
#include "fourier_aurora_sdk/transport/transport.hpp"

namespace fourier::aurora_sdk {

class FourierDdsTransport final : public ITransport {
public:
    FourierDdsTransport(uint32_t domain_id, const NormalizedConnectionOptions& options);
    ~FourierDdsTransport() override;

    FourierDdsTransport(const FourierDdsTransport&) = delete;
    FourierDdsTransport& operator=(const FourierDdsTransport&) = delete;

    void setAuroraStateHandler(AuroraStateHandler handler) override;
    void setMotionStateHandler(MotionStateHandler handler) override;
    void setControlGroupStateHandler(ControlGroupStateHandler handler) override;
    void setEstimatorDataHandler(EstimatorDataHandler handler) override;
    void setErrorCodesHandler(ErrorCodesHandler handler) override;

    void publishMotionCommand(const MotionCommand& command) override;
    void publishControlGroupCommand(const ControlGroupCommand& command) override;
    void publishSubControllerCommand(const SubControllerCommand& command) override;
    ServiceCallResult requestFsmState(uint32_t state_id, uint32_t timeout_ms) override;

    bool isMotionCommandPublisherMatched() const override;
    bool isControlGroupCommandPublisherMatched() const override;
    bool isSubControllerCommandPublisherMatched() const override;
    bool isAuroraStateSubscriberMatched() const override;
    bool isMotionStateSubscriberMatched() const override;
    bool isControlGroupStateSubscriberMatched() const override;
    bool isEstimatorDataSubscriberMatched() const override;
    bool isErrorCodesSubscriberMatched() const override;

    void close() override;

private:
    struct Impl;
    std::unique_ptr<Impl> impl_;
};

}  // namespace fourier::aurora_sdk
