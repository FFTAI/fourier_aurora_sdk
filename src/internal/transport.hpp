#pragma once

#include <functional>
#include <memory>
#include <string>
#include <vector>

#include "fourier_aurora_sdk/callbacks.hpp"
#include "fourier_aurora_sdk/connection_options.hpp"
#include "fourier_aurora_sdk/types.hpp"

namespace fourier::aurora_sdk::detail {

struct TransportHandlers {
    AuroraStateHandler aurora_state;
    MotionStateHandler motion_state;
    ControlGroupStateHandler control_group_state;
    MotorConfigurationStateHandler motor_configuration_state;
    EstimatorDataHandler estimator_data;
    ErrorCodesHandler error_codes;
};

using TransportTerminalObserver = std::function<void()>;

struct LeaseToken {
    uint32_t source_id{0};
    uint64_t lease_id{0};
    uint64_t sequence_id{0};
};

struct LeaseManagementRequest {
    uint32_t source_id{0};
    std::string source_name;
    uint64_t client_instance_id{0};
    uint32_t action{0};
    uint64_t operation_id{0};
    uint64_t lease_id{0};
    std::vector<std::string> resources;
    uint32_t priority{0};
    double renew_duration{0.0};
};

struct LeaseManagementResponse {
    OperationResult operation;
    uint32_t wire_result{0};
    uint64_t lease_id{0};
    double renew_duration{0.0};
    std::string message;
};

class ITransport {
public:
    virtual ~ITransport() = default;

    virtual OperationResult publishMotionCommand(
        const MotionCommand& command, const LeaseToken& lease) = 0;
    virtual OperationResult publishControlGroupCommand(
        const ControlGroupCommand& command, const LeaseToken& lease) = 0;
    virtual OperationResult publishMotorConfigurationCommand(
        const MotorConfigurationCommand& command, const LeaseToken& lease) = 0;
    virtual OperationResult setSubController(
        const SubControllerCommand& command) = 0;
    virtual LeaseManagementResponse manageLease(
        const LeaseManagementRequest& request, uint32_t timeout_ms) = 0;
    virtual OperationResult requestFsmState(uint32_t state_id) = 0;

    [[nodiscard]] virtual EndpointMatchStatus endpointMatchStatus() const = 0;
    virtual void stop() = 0;
};

[[nodiscard]] std::unique_ptr<ITransport> makeProductionTransport(
    const ConnectionOptions& options,
    TransportHandlers handlers,
    TransportTerminalObserver terminal_observer);

#if defined(FOURIER_AURORA_SDK_TESTING)
using TransportFactory = std::function<std::unique_ptr<ITransport>(
    const ConnectionOptions&,
    TransportHandlers,
    TransportTerminalObserver)>;

void setTransportFactoryForTesting(TransportFactory factory);
void resetTransportFactoryForTesting();
#endif

}  // namespace fourier::aurora_sdk::detail
