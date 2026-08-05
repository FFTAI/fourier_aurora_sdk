#pragma once

#include "fourier_aurora_sdk/aurora_client.hpp"
#include "mock_transport.hpp"

namespace fourier::aurora_sdk::testing {

inline AuroraClient& resetClientFixture() {
    auto& client = AuroraClient::getInstance();
    try {
        client.stop();
    } catch (...) {
    }
    client.onAuroraState({});
    client.onMotionState({});
    client.onControlGroupState({});
    client.onMotorConfigurationState({});
    client.onEstimatorData({});
    client.onErrorCodes({});
    client.onLeaseChanged({});
    client.setDomainId(123);
    ConnectionOptions options;
    options.source_id = 42;
    client.configure(options);
    resetMockTransportFixture();
    return client;
}

}  // namespace fourier::aurora_sdk::testing
