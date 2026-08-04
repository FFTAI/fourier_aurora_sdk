#include "fourier_aurora_sdk/aurora_client.hpp"
#include "mock_transport.hpp"

int main() {
    using namespace fourier::aurora_sdk;
    using namespace fourier::aurora_sdk::testing;

    resetMockTransportFixture();
    auto& client = AuroraClient::getInstance();
    client.setDomainId(123);
    auto options = client.config();
    options.source_id = 77;
    options.source_name = "destructor_fallback_test";
    client.configure(options);
    client.start();

    // Intentionally omit stop(). CTest bounds this process so fallback
    // destruction regressions are reported as a timeout.
    return 0;
}
