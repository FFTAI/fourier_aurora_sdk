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
    client.onEstimatorData({});
    client.onErrorCodes({});
    client.setDomainId(123);
    client.setUserId("aurora_client");
    client.configure(ConnectionOptions{});
    resetMockTransportFixture();
    return client;
}

}  // namespace fourier::aurora_sdk::testing
