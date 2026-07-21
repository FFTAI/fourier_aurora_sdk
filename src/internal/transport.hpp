#pragma once

#include <functional>
#include <memory>

#include "fourier_aurora_sdk/callbacks.hpp"
#include "fourier_aurora_sdk/connection_options.hpp"
#include "fourier_aurora_sdk/types.hpp"

namespace fourier::aurora_sdk::detail {

struct TransportHandlers {
    AuroraStateHandler aurora_state;
    MotionStateHandler motion_state;
    ControlGroupStateHandler control_group_state;
    EstimatorDataHandler estimator_data;
    ErrorCodesHandler error_codes;
};

using TransportTerminalObserver = std::function<void()>;

class ITransport {
public:
    virtual ~ITransport() = default;

    virtual OperationResult publishMotionCommand(const MotionCommand& command) = 0;
    virtual OperationResult publishControlGroupCommand(
        const ControlGroupCommand& command) = 0;
    virtual OperationResult publishSubControllerCommand(
        const SubControllerCommand& command) = 0;
    virtual OperationResult requestFsmState(uint32_t state_id) = 0;

    [[nodiscard]] virtual EndpointMatchStatus endpointMatchStatus() const = 0;
    virtual void stop() = 0;
};

[[nodiscard]] std::unique_ptr<ITransport> makeProductionTransport(
    uint32_t domain_id,
    const ConnectionOptions& options,
    TransportHandlers handlers,
    TransportTerminalObserver terminal_observer);

#if defined(FOURIER_AURORA_SDK_TESTING)
using TransportFactory = std::function<std::unique_ptr<ITransport>(
    uint32_t,
    const ConnectionOptions&,
    TransportHandlers,
    TransportTerminalObserver)>;

void setTransportFactoryForTesting(TransportFactory factory);
void resetTransportFactoryForTesting();
#endif

}  // namespace fourier::aurora_sdk::detail
