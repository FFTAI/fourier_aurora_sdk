#include <atomic>
#include <stdexcept>

#include "fourier_aurora_sdk/errors.hpp"
#include "internal/teardown_coordinator.hpp"
#include "test_harness.hpp"

namespace sdk = fourier::aurora_sdk;
namespace detail = fourier::aurora_sdk::detail;
namespace testing = fourier::aurora_sdk::testing;

int main() {
    testing::TestHarness test;
    auto& coordinator = detail::TeardownCoordinator::instance();
    coordinator.ensureWorker();

    auto standard_failure = coordinator.submit([] {
        throw std::runtime_error("injected coordinator job failure");
    });
    SDK_CHECK_THROWS(test, std::runtime_error, standard_failure.get());

    auto nonstandard_failure = coordinator.submit([] { throw 7; });
    SDK_CHECK_THROWS(test, int, nonstandard_failure.get());

    std::atomic<bool> later_job_ran{false};
    auto later_job = coordinator.submit([&] { later_job_ran = true; });
    later_job.get();
    SDK_CHECK(test, later_job_ran);

    std::atomic<bool> recursive_rejected{false};
    auto recursive = coordinator.submit([&] {
        SDK_CHECK(test, coordinator.isWorkerThread());
        try {
            static_cast<void>(coordinator.submit([] {}));
        } catch (const sdk::ConnectionError&) {
            recursive_rejected = true;
        }
    });
    recursive.get();
    SDK_CHECK(test, recursive_rejected);

    return test.result();
}
