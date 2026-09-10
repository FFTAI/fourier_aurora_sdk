#include <atomic>
#include <chrono>
#include <condition_variable>
#include <exception>
#include <mutex>
#include <sstream>
#include <stdexcept>
#include <thread>

#include "client_fixture.hpp"
#include "test_harness.hpp"

namespace sdk = fourier::aurora_sdk;
namespace testing = fourier::aurora_sdk::testing;

namespace {

using namespace std::chrono_literals;

template <typename Predicate>
bool waitUntil(Predicate&& predicate, std::chrono::milliseconds timeout) {
    const auto deadline = std::chrono::steady_clock::now() + timeout;
    do {
        if (predicate()) {
            return true;
        }
        std::this_thread::sleep_for(1ms);
    } while (std::chrono::steady_clock::now() < deadline);
    return predicate();
}

void testConcurrentLifecycle(testing::TestHarness& test) {
    auto& client = testing::resetClientFixture();
    auto& control = testing::mockFactoryControl();
    {
        std::lock_guard<std::mutex> lock(control.mutex);
        control.block_create = true;
        control.release_create = false;
    }

    std::exception_ptr first_error;
    std::exception_ptr second_error;
    std::thread first([&] {
        try {
            client.start();
        } catch (...) {
            first_error = std::current_exception();
        }
    });
    SDK_CHECK(test, control.waitForCreateCalls(1, 1s));
    SDK_CHECK(test, client.state() == sdk::ClientState::Starting);
    std::thread second([&] {
        try {
            client.start();
        } catch (...) {
            second_error = std::current_exception();
        }
    });
    std::this_thread::sleep_for(20ms);
    {
        std::lock_guard<std::mutex> lock(control.mutex);
        control.release_create = true;
        control.changed.notify_all();
    }
    first.join();
    second.join();
    SDK_CHECK(test, !first_error);
    SDK_CHECK(test, !second_error);
    SDK_CHECK(test, client.state() == sdk::ClientState::Running);
    {
        std::lock_guard<std::mutex> lock(control.mutex);
        SDK_CHECK(test, control.create_calls == 1);
    }
    client.start();
    {
        std::lock_guard<std::mutex> lock(control.mutex);
        SDK_CHECK(test, control.create_calls == 1);
    }
    SDK_CHECK_THROWS(
        test,
        sdk::InvalidStateError,
        client.setDomainId(99));
    SDK_CHECK_THROWS(
        test,
        sdk::InvalidStateError,
        client.configure(client.config()));
    client.stop();
    client.stop();
}

void testOpposingTransitions(testing::TestHarness& test) {
    auto& client = testing::resetClientFixture();
    auto& control = testing::mockFactoryControl();
    {
        std::lock_guard<std::mutex> lock(control.mutex);
        control.block_create = true;
        control.release_create = false;
    }

    std::thread starter([&] { client.start(); });
    SDK_CHECK(test, control.waitForCreateCalls(1, 1s));
    std::thread stopper([&] { client.stop(); });
    std::this_thread::sleep_for(20ms);
    {
        std::lock_guard<std::mutex> lock(control.mutex);
        control.release_create = true;
        control.changed.notify_all();
    }
    starter.join();
    stopper.join();
    SDK_CHECK(test, client.state() == sdk::ClientState::Stopped);

    testing::resetMockTransportFixture();
    client.start();
    auto first_session = control.latest();
    {
        std::lock_guard<std::mutex> lock(first_session->mutex);
        first_session->block_stop = true;
        first_session->release_stop = false;
    }

    std::thread stopping([&] { client.stop(); });
    SDK_CHECK(
        test,
        waitUntil(
            [&] {
                std::lock_guard<std::mutex> lock(first_session->mutex);
                return first_session->stop_calls == 1;
            },
            1s));
    SDK_CHECK(test, client.state() == sdk::ClientState::Stopping);
    std::thread starting([&] { client.start(); });
    std::this_thread::sleep_for(20ms);
    {
        std::lock_guard<std::mutex> lock(first_session->mutex);
        first_session->release_stop = true;
        first_session->changed.notify_all();
    }
    stopping.join();
    starting.join();
    SDK_CHECK(test, client.state() == sdk::ClientState::Running);
    {
        std::lock_guard<std::mutex> lock(control.mutex);
        SDK_CHECK(test, control.create_calls == 2);
    }
    client.stop();
}

void testSharedTransitionFailures(testing::TestHarness& test) {
    auto& client = testing::resetClientFixture();
    auto& control = testing::mockFactoryControl();
    {
        std::lock_guard<std::mutex> lock(control.mutex);
        control.block_create = true;
        control.release_create = false;
        control.fail_create = true;
    }

    std::atomic<int> connection_errors{0};
    auto start = [&] {
        try {
            client.start();
        } catch (const sdk::ConnectionError&) {
            ++connection_errors;
        }
    };
    std::thread first(start);
    SDK_CHECK(test, control.waitForCreateCalls(1, 1s));
    std::thread second(start);
    std::this_thread::sleep_for(20ms);
    {
        std::lock_guard<std::mutex> lock(control.mutex);
        control.release_create = true;
        control.changed.notify_all();
    }
    first.join();
    second.join();
    SDK_CHECK(test, connection_errors == 2);
    SDK_CHECK(test, client.state() == sdk::ClientState::Stopped);
    {
        std::lock_guard<std::mutex> lock(control.mutex);
        SDK_CHECK(test, control.create_calls == 1);
    }

    testing::resetMockTransportFixture();
    client.start();
    auto session = control.latest();
    {
        std::lock_guard<std::mutex> lock(session->mutex);
        session->block_stop = true;
        session->release_stop = false;
        session->throw_stop = true;
    }
    connection_errors = 0;
    auto stop = [&] {
        try {
            client.stop();
        } catch (const sdk::ConnectionError&) {
            ++connection_errors;
        }
    };
    std::thread stop_first(stop);
    SDK_CHECK(
        test,
        waitUntil(
            [&] {
                std::lock_guard<std::mutex> lock(session->mutex);
                return session->stop_calls == 1;
            },
            1s));
    std::thread stop_second(stop);
    std::this_thread::sleep_for(20ms);
    {
        std::lock_guard<std::mutex> lock(session->mutex);
        session->release_stop = true;
        session->changed.notify_all();
    }
    stop_first.join();
    stop_second.join();
    SDK_CHECK(test, connection_errors == 2);
    SDK_CHECK(test, client.state() == sdk::ClientState::Stopped);
}

void testOperationAndCallbackDrain(testing::TestHarness& test) {
    auto& client = testing::resetClientFixture();
    auto options = client.config();
    options.enabled_endpoints = {
        sdk::Endpoint::FsmService,
        sdk::Endpoint::MotionStateSubscriber,
    };
    client.configure(options);

    std::mutex callback_mutex;
    std::condition_variable callback_changed;
    bool callback_entered = false;
    bool release_callback = false;
    client.onMotionState([&](sdk::MotionState) {
        std::unique_lock<std::mutex> lock(callback_mutex);
        callback_entered = true;
        callback_changed.notify_all();
        callback_changed.wait(lock, [&] { return release_callback; });
    });
    client.start();
    auto session = testing::mockFactoryControl().latest();

    std::thread callback_thread([&] {
        session->injectMotionState({});
    });
    SDK_CHECK(
        test,
        waitUntil(
            [&] {
                std::lock_guard<std::mutex> lock(callback_mutex);
                return callback_entered;
            },
            1s));
    std::atomic<bool> stop_returned{false};
    std::thread stopper([&] {
        client.stop();
        stop_returned = true;
    });
    std::this_thread::sleep_for(20ms);
    SDK_CHECK(test, client.state() == sdk::ClientState::Stopping);
    SDK_CHECK(test, !stop_returned);
    {
        std::lock_guard<std::mutex> lock(callback_mutex);
        release_callback = true;
        callback_changed.notify_all();
    }
    callback_thread.join();
    stopper.join();
    SDK_CHECK(test, stop_returned);
    SDK_CHECK(test, client.state() == sdk::ClientState::Stopped);

    testing::resetMockTransportFixture();
    client.onMotionState({});
    client.start();
    session = testing::mockFactoryControl().latest();
    {
        std::lock_guard<std::mutex> lock(session->mutex);
        session->block_request = true;
        session->release_request = false;
    }
    sdk::OperationResult request_result;
    std::thread request([&] {
        request_result = client.requestFsmState(13);
    });
    SDK_CHECK(
        test,
        waitUntil(
            [&] {
                std::lock_guard<std::mutex> lock(session->mutex);
                return session->request_calls == 1;
            },
            1s));
    stop_returned = false;
    std::thread operation_stopper([&] {
        client.stop();
        stop_returned = true;
    });
    std::this_thread::sleep_for(20ms);
    SDK_CHECK(test, !stop_returned);
    {
        std::lock_guard<std::mutex> lock(session->mutex);
        session->release_request = true;
        session->changed.notify_all();
    }
    request.join();
    operation_stopper.join();
    SDK_CHECK(test, request_result.success());
    SDK_CHECK(test, stop_returned);
}

void testCallbackBoundaries(testing::TestHarness& test) {
    auto& client = testing::resetClientFixture();
    auto options = client.config();
    options.enabled_endpoints = {
        sdk::Endpoint::MotionStateSubscriber,
    };
    client.configure(options);

    bool stop_rejected = false;
    client.onMotionState([&](sdk::MotionState) {
        try {
            client.stop();
        } catch (const sdk::InvalidStateError&) {
            stop_rejected = true;
        }
    });
    client.start();
    auto session = testing::mockFactoryControl().latest();
    session->injectMotionState({});
    SDK_CHECK(test, stop_rejected);
    SDK_CHECK(test, client.state() == sdk::ClientState::Running);
    client.stop();

    testing::resetMockTransportFixture();
    client.onMotionState([](sdk::MotionState) {
        throw std::runtime_error("injected callback error");
    });
    std::ostringstream standard_log;
    auto* old_buffer = std::cerr.rdbuf(standard_log.rdbuf());
    client.start();
    session = testing::mockFactoryControl().latest();
    session->injectMotionState({});
    client.stop();
    std::cerr.rdbuf(old_buffer);
    SDK_CHECK(
        test,
        standard_log.str().find("[fourier_aurora_sdk]") !=
            std::string::npos);
    SDK_CHECK(
        test,
        standard_log.str().find("injected callback error") !=
            std::string::npos);

    testing::resetMockTransportFixture();
    client.onMotionState([](sdk::MotionState) { throw 5; });
    std::ostringstream nonstandard_log;
    old_buffer = std::cerr.rdbuf(nonstandard_log.rdbuf());
    client.start();
    session = testing::mockFactoryControl().latest();
    session->injectMotionState({});
    client.stop();
    std::cerr.rdbuf(old_buffer);
    SDK_CHECK(
        test,
        nonstandard_log.str().find("non-standard") !=
            std::string::npos);
}

void testStartingSamplesAndRestartPersistence(
    testing::TestHarness& test) {
    auto& client = testing::resetClientFixture();
    auto options = client.config();
    options.enabled_endpoints = {
        sdk::Endpoint::MotionStateSubscriber,
    };
    client.configure(options);
    int callback_count = 0;
    client.onMotionState([&](sdk::MotionState) { ++callback_count; });

    auto& control = testing::mockFactoryControl();
    {
        std::lock_guard<std::mutex> lock(control.mutex);
        control.inject_motion_during_create = true;
        control.create_motion_sample.stable_level = 41;
    }
    client.start();
    auto session = control.latest();
    sdk::MotionState output;
    SDK_CHECK(test, client.getMotionState(output).success());
    SDK_CHECK(test, output.stable_level == 41);
    SDK_CHECK(test, callback_count == 0);
    sdk::MotionState running_sample;
    running_sample.stable_level = 42;
    session->injectMotionState(running_sample);
    SDK_CHECK(test, callback_count == 1);
    client.stop();

    testing::resetMockTransportFixture();
    client.start();
    session = control.latest();
    session->injectMotionState(running_sample);
    SDK_CHECK(test, callback_count == 2);
    client.stop();
}

void testFatalConvergence(testing::TestHarness& test) {
    auto& client = testing::resetClientFixture();
    auto options = client.config();
    options.enabled_endpoints = {sdk::Endpoint::FsmService};
    client.configure(options);
    client.start();
    auto session = testing::mockFactoryControl().latest();
    {
        std::lock_guard<std::mutex> lock(session->mutex);
        session->fatal_request = true;
    }
    SDK_CHECK_THROWS(
        test, sdk::ConnectionError, client.requestFsmState(13));
    SDK_CHECK(test, client.state() == sdk::ClientState::Stopped);
    client.stop();
}

}  // namespace

int main() {
    testing::TestHarness test;
    testConcurrentLifecycle(test);
    testOpposingTransitions(test);
    testSharedTransitionFailures(test);
    testOperationAndCallbackDrain(test);
    testCallbackBoundaries(test);
    testStartingSamplesAndRestartPersistence(test);
    testFatalConvergence(test);
    return test.result();
}
