#pragma once

#include <functional>
#include <memory>

#include "fourier_aurora_sdk/callbacks.hpp"
#include "fourier_aurora_sdk/connection_options.hpp"
#include "fourier_aurora_sdk/types.hpp"

namespace fourier::aurora_sdk {

namespace detail {
class FourierDdsTransportFactory;
struct LeaseToken;
struct LeaseManagementRequest;
struct LeaseManagementResponse;
}

class FourierDdsTransport final {
public:
    explicit FourierDdsTransport(
        uint32_t domain_id,
        ConnectionOptions options = {});
    ~FourierDdsTransport() noexcept;

    FourierDdsTransport(const FourierDdsTransport&) = delete;
    FourierDdsTransport& operator=(const FourierDdsTransport&) = delete;
    FourierDdsTransport(FourierDdsTransport&&) = delete;
    FourierDdsTransport& operator=(FourierDdsTransport&&) = delete;

    void setAuroraStateHandler(AuroraStateHandler handler);
    void setMotionStateHandler(MotionStateHandler handler);
    void setControlGroupStateHandler(ControlGroupStateHandler handler);
    void setMotorConfigurationStateHandler(MotorConfigurationStateHandler handler);
    void setEstimatorDataHandler(EstimatorDataHandler handler);
    void setErrorCodesHandler(ErrorCodesHandler handler);

    [[nodiscard]] OperationResult publishMotionCommand(
        const MotionCommand& command, const detail::LeaseToken& lease);
    [[nodiscard]] OperationResult publishControlGroupCommand(
        const ControlGroupCommand& command, const detail::LeaseToken& lease);
    [[nodiscard]] OperationResult publishMotorConfigurationCommand(
        const MotorConfigurationCommand& command, const detail::LeaseToken& lease);
    [[nodiscard]] OperationResult setSubController(
        const SubControllerCommand& command);
    [[nodiscard]] detail::LeaseManagementResponse manageLease(
        const detail::LeaseManagementRequest& request, uint32_t timeout_ms);
    [[nodiscard]] OperationResult requestFsmState(uint32_t state_id);

    [[nodiscard]] EndpointMatchStatus endpointMatchStatus() const;
    [[nodiscard]] bool isFsmServiceMatched() const;
    [[nodiscard]] bool isMotionCommandPublisherMatched() const;
    [[nodiscard]] bool isControlGroupCommandPublisherMatched() const;
    [[nodiscard]] bool isMotorConfigurationCommandPublisherMatched() const;
    [[nodiscard]] bool isSubControllerServiceMatched() const;
    [[nodiscard]] bool isManageLeaseServiceMatched() const;
    [[nodiscard]] bool isAuroraStateSubscriberMatched() const;
    [[nodiscard]] bool isMotionStateSubscriberMatched() const;
    [[nodiscard]] bool isControlGroupStateSubscriberMatched() const;
    [[nodiscard]] bool isMotorConfigurationStateSubscriberMatched() const;
    [[nodiscard]] bool isEstimatorDataSubscriberMatched() const;
    [[nodiscard]] bool isErrorCodesSubscriberMatched() const;

    void stop();

private:
    struct Impl;
    struct InitialHandlers;

    FourierDdsTransport(
        uint32_t domain_id,
        ConnectionOptions options,
        InitialHandlers handlers);

    friend class detail::FourierDdsTransportFactory;

    std::shared_ptr<Impl> impl_;
};

}  // namespace fourier::aurora_sdk
