#include <type_traits>

#include "fourier_aurora_sdk/aurora_client.hpp"
#include "fourier_aurora_sdk/transport/fourierdds_transport.hpp"
#include "internal/transport.hpp"
#include "test_harness.hpp"

namespace sdk = fourier::aurora_sdk;
namespace testing = fourier::aurora_sdk::testing;

int main() {
    testing::TestHarness test;
    static_assert(!std::is_copy_constructible_v<sdk::FourierDdsTransport>);
    static_assert(!std::is_move_constructible_v<sdk::FourierDdsTransport>);

    sdk::detail::resetTransportFactoryForTesting();
    auto& client = sdk::AuroraClient::getInstance();
    client.stop();
    SDK_CHECK_THROWS(test, sdk::InvalidArgumentError, client.start());
    SDK_CHECK(test, client.state() == sdk::ClientState::Stopped);
    client.setDomainId(123);
    auto options = client.config();
    options.source_name = "dds-disabled-test";
    client.configure(options);

    SDK_CHECK_THROWS(test, sdk::ConnectionError, client.start());
    SDK_CHECK(test, client.state() == sdk::ClientState::Stopped);
    SDK_CHECK_THROWS(
        test,
        sdk::ConnectionError,
        sdk::FourierDdsTransport(options));
    client.stop();
    return test.result();
}
