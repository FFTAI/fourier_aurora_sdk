#include <pybind11/pybind11.h>

#include <chrono>
#include <memory>
#include <mutex>
#include <stdexcept>
#include <string>

#include "client_fixture.hpp"

namespace py = pybind11;
namespace sdk = fourier::aurora_sdk;
namespace testing = fourier::aurora_sdk::testing;

namespace {

std::shared_ptr<testing::MockSession> requireSession() {
    auto session = testing::mockFactoryControl().latest();
    if (!session) {
        throw std::runtime_error("mock transport session is not available");
    }
    return session;
}

}  // namespace

void bind_test_fixture(py::module_& module) {
    module.def("_testing_reset", [] {
        static_cast<void>(testing::resetClientFixture());
    });
    module.def("_testing_set_match_all", [](bool matched) {
        requireSession()->setAllEnabledMatched(matched);
    });
    module.def("_testing_inject_motion_state", [](uint8_t stable_level) {
        sdk::MotionState state;
        state.stable_level = stable_level;
        requireSession()->injectMotionState(std::move(state));
    });
    module.def("_testing_inject_estimator_data", [](const sdk::EstimatorData& state) {
        requireSession()->injectEstimatorData(state);
    });
    module.def(
        "_testing_set_fsm_result",
        [](uint32_t code, const std::string& message) {
            auto session = requireSession();
            std::lock_guard<std::mutex> lock(session->mutex);
            session->fsm_result = {code, message};
        });
    module.def("_testing_set_motion_publish_result", [](uint32_t code) {
        auto session = requireSession();
        std::lock_guard<std::mutex> lock(session->mutex);
        session->motion_publish_result = {
            code,
            std::string(sdk::defaultResultMessage(code)),
        };
    });
    module.def("_testing_block_create", [] {
        auto& control = testing::mockFactoryControl();
        std::lock_guard<std::mutex> lock(control.mutex);
        control.block_create = true;
        control.release_create = false;
    });
    module.def("_testing_release_create", [] {
        auto& control = testing::mockFactoryControl();
        std::lock_guard<std::mutex> lock(control.mutex);
        control.release_create = true;
        control.changed.notify_all();
    });
    module.def(
        "_testing_wait_create",
        [](std::size_t count, double seconds) {
            py::gil_scoped_release release;
            return testing::mockFactoryControl().waitForCreateCalls(
                count,
                std::chrono::duration_cast<std::chrono::milliseconds>(
                    std::chrono::duration<double>(seconds)));
        });
    module.def("_testing_block_stop", [] {
        auto session = requireSession();
        std::lock_guard<std::mutex> lock(session->mutex);
        session->block_stop = true;
        session->release_stop = false;
    });
    module.def("_testing_release_stop", [] {
        auto session = requireSession();
        std::lock_guard<std::mutex> lock(session->mutex);
        session->release_stop = true;
        session->changed.notify_all();
    });
    module.def("_testing_wait_stop", [](std::size_t count, double seconds) {
        auto session = requireSession();
        py::gil_scoped_release release;
        std::unique_lock<std::mutex> lock(session->mutex);
        return session->changed.wait_for(
            lock,
            std::chrono::duration<double>(seconds),
            [&] { return session->stop_calls >= count; });
    });
    module.def("_testing_block_request", [] {
        auto session = requireSession();
        std::lock_guard<std::mutex> lock(session->mutex);
        session->block_request = true;
        session->release_request = false;
    });
    module.def("_testing_release_request", [] {
        auto session = requireSession();
        std::lock_guard<std::mutex> lock(session->mutex);
        session->release_request = true;
        session->changed.notify_all();
    });
    module.def(
        "_testing_wait_request",
        [](std::size_t count, double seconds) {
            auto session = requireSession();
            py::gil_scoped_release release;
            std::unique_lock<std::mutex> lock(session->mutex);
            return session->changed.wait_for(
                lock,
                std::chrono::duration<double>(seconds),
                [&] { return session->request_calls >= count; });
        });
}
