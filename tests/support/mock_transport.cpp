#include "mock_transport.hpp"

#include <chrono>
#include <stdexcept>
#include <utility>

#include "fourier_aurora_sdk/errors.hpp"

namespace fourier::aurora_sdk::testing {

namespace {

EndpointStatus& endpointStatus(
    EndpointMatchStatus& status,
    Endpoint endpoint) {
    switch (endpoint) {
        case Endpoint::FsmService:
            return status.fsm_service;
        case Endpoint::ManageLeaseService:
            return status.manage_lease_service;
        case Endpoint::MotionCommandPublisher:
            return status.motion_command_publisher;
        case Endpoint::ControlGroupCommandPublisher:
            return status.control_group_command_publisher;
        case Endpoint::MotorConfigurationCommandPublisher:
            return status.motor_configuration_command_publisher;
        case Endpoint::SubControllerService:
            return status.sub_controller_service;
        case Endpoint::AuroraStateSubscriber:
            return status.aurora_state_subscriber;
        case Endpoint::MotionStateSubscriber:
            return status.motion_state_subscriber;
        case Endpoint::ControlGroupStateSubscriber:
            return status.control_group_state_subscriber;
        case Endpoint::MotorConfigurationStateSubscriber:
            return status.motor_configuration_state_subscriber;
        case Endpoint::EstimatorDataSubscriber:
            return status.estimator_data_subscriber;
        case Endpoint::ErrorCodesSubscriber:
            return status.error_codes_subscriber;
    }
    throw InvalidArgumentError("invalid Endpoint value");
}

OperationResult notRunningResult() {
    return {
        result_codes::CLIENT_NOT_RUNNING,
        std::string(defaultResultMessage(
            result_codes::CLIENT_NOT_RUNNING)),
    };
}

class MockTransport final : public detail::ITransport {
public:
    explicit MockTransport(std::shared_ptr<MockSession> session)
        : session_(std::move(session)) {}

    OperationResult publishMotionCommand(
        const MotionCommand& command, const detail::LeaseToken& token) override {
        std::unique_lock<std::mutex> lock(session_->mutex);
        if (session_->stopped) {
            return notRunningResult();
        }
        session_->motion_commands.push_back(command);
        session_->motion_tokens.push_back(token);
        if (session_->fatal_publish) {
            const auto observer = session_->terminal_observer;
            lock.unlock();
            if (observer) {
                observer();
            }
            throw ConnectionError("injected fatal publish failure");
        }
        return session_->motion_publish_result;
    }

    OperationResult publishControlGroupCommand(
        const ControlGroupCommand& command, const detail::LeaseToken& token) override {
        std::unique_lock<std::mutex> lock(session_->mutex);
        if (session_->stopped) {
            return notRunningResult();
        }
        session_->control_group_commands.push_back(command);
        session_->control_group_tokens.push_back(token);
        if (session_->fatal_publish) {
            const auto observer = session_->terminal_observer;
            lock.unlock();
            if (observer) {
                observer();
            }
            throw ConnectionError("injected fatal publish failure");
        }
        return session_->control_group_publish_result;
    }

    OperationResult publishMotorConfigurationCommand(
        const MotorConfigurationCommand& command,
        const detail::LeaseToken& token) override {
        std::lock_guard<std::mutex> lock(session_->mutex);
        session_->motor_configuration_commands.push_back(command);
        session_->motor_configuration_tokens.push_back(token);
        return session_->motor_configuration_publish_result;
    }

    OperationResult setSubController(
        const SubControllerCommand& command) override {
        std::unique_lock<std::mutex> lock(session_->mutex);
        if (session_->stopped) {
            return notRunningResult();
        }
        session_->sub_controller_commands.push_back(command);
        if (session_->fatal_publish) {
            const auto observer = session_->terminal_observer;
            lock.unlock();
            if (observer) {
                observer();
            }
            throw ConnectionError("injected fatal publish failure");
        }
        return session_->sub_controller_result;
    }

    detail::LeaseManagementResponse manageLease(
        const detail::LeaseManagementRequest& request,
        uint32_t) override {
        std::lock_guard<std::mutex> lock(session_->mutex);
        session_->lease_requests.push_back(request);
        auto response = session_->lease_response;
        if (response.operation.success() && response.wire_result == 0) {
            response.lease_id = request.action == 0 || request.action == 3
                ? (session_->lease_requests.size() + 100)
                : request.lease_id;
            response.renew_duration = request.action == 2
                ? 0.0 : request.renew_duration;
        }
        return response;
    }

    OperationResult requestFsmState(uint32_t state_id) override {
        std::unique_lock<std::mutex> lock(session_->mutex);
        if (session_->stopped) {
            return notRunningResult();
        }
        ++session_->request_calls;
        session_->fsm_requests.push_back(state_id);
        session_->changed.notify_all();
        session_->changed.wait(lock, [this] {
            return !session_->block_request || session_->release_request;
        });
        if (session_->fatal_request) {
            const auto observer = session_->terminal_observer;
            lock.unlock();
            if (observer) {
                observer();
            }
            throw ConnectionError("injected fatal request failure");
        }
        return session_->fsm_result;
    }

    EndpointMatchStatus endpointMatchStatus() const override {
        std::lock_guard<std::mutex> lock(session_->mutex);
        if (session_->stopped) {
            auto status = session_->match_status;
            for (const auto endpoint : session_->options.enabled_endpoints) {
                endpointStatus(status, endpoint).matched = false;
            }
            return status;
        }
        return session_->match_status;
    }

    void stop() override {
        detail::TransportTerminalObserver observer;
        bool throw_standard = false;
        bool throw_nonstandard = false;
        {
            std::unique_lock<std::mutex> lock(session_->mutex);
            ++session_->stop_calls;
            session_->changed.notify_all();
            session_->changed.wait(lock, [this] {
                return !session_->block_stop || session_->release_stop;
            });
            session_->stopped = true;
            session_->stop_thread = std::this_thread::get_id();
            observer = session_->terminal_observer;
            throw_standard = session_->throw_stop;
            throw_nonstandard = session_->throw_nonstandard_stop;
            session_->changed.notify_all();
        }
        if (observer) {
            observer();
        }
        if (throw_nonstandard) {
            throw 7;
        }
        if (throw_standard) {
            throw ConnectionError("injected teardown failure");
        }
    }

private:
    std::shared_ptr<MockSession> session_;
};

void initializeMatchStatus(MockSession& session, bool matched) {
    for (const auto endpoint : session.options.enabled_endpoints) {
        auto& status = endpointStatus(session.match_status, endpoint);
        status.enabled = true;
        status.matched = matched;
    }
}

}  // namespace

void MockSession::setMatched(Endpoint endpoint, bool matched) {
    std::lock_guard<std::mutex> lock(mutex);
    auto& status = endpointStatus(match_status, endpoint);
    if (!status.enabled) {
        throw InvalidStateError("cannot match a disabled mock endpoint");
    }
    status.matched = matched;
    changed.notify_all();
}

void MockSession::setAllEnabledMatched(bool matched) {
    std::lock_guard<std::mutex> lock(mutex);
    for (const auto endpoint : options.enabled_endpoints) {
        endpointStatus(match_status, endpoint).matched = matched;
    }
    changed.notify_all();
}

void MockSession::injectAuroraState(AuroraState state) {
    AuroraStateHandler handler;
    {
        std::lock_guard<std::mutex> lock(mutex);
        if (stopped) {
            return;
        }
        handler = handlers.aurora_state;
    }
    if (handler) {
        handler(std::move(state));
    }
}

void MockSession::injectMotionState(MotionState state) {
    MotionStateHandler handler;
    {
        std::lock_guard<std::mutex> lock(mutex);
        if (stopped) {
            return;
        }
        handler = handlers.motion_state;
    }
    if (handler) {
        handler(std::move(state));
    }
}

void MockSession::injectControlGroupState(ControlGroupState state) {
    ControlGroupStateHandler handler;
    {
        std::lock_guard<std::mutex> lock(mutex);
        if (stopped) {
            return;
        }
        handler = handlers.control_group_state;
    }
    if (handler) {
        handler(std::move(state));
    }
}

void MockSession::injectEstimatorData(EstimatorData data) {
    EstimatorDataHandler handler;
    {
        std::lock_guard<std::mutex> lock(mutex);
        if (stopped) {
            return;
        }
        handler = handlers.estimator_data;
    }
    if (handler) {
        handler(std::move(data));
    }
}

void MockSession::injectErrorCodes(ErrorCodes codes) {
    ErrorCodesHandler handler;
    {
        std::lock_guard<std::mutex> lock(mutex);
        if (stopped) {
            return;
        }
        handler = handlers.error_codes;
    }
    if (handler) {
        handler(std::move(codes));
    }
}

void MockFactoryControl::reset() {
    std::lock_guard<std::mutex> lock(mutex);
    block_create = false;
    release_create = true;
    fail_create = false;
    throw_nonstandard_create = false;
    match_on_create = true;
    inject_motion_during_create = false;
    create_motion_sample = {};
    create_calls = 0;
    latest_session.reset();
    changed.notify_all();
}

std::shared_ptr<MockSession> MockFactoryControl::latest() const {
    std::lock_guard<std::mutex> lock(mutex);
    return latest_session;
}

bool MockFactoryControl::waitForCreateCalls(
    std::size_t count,
    std::chrono::milliseconds timeout) {
    const auto deadline = std::chrono::steady_clock::now() + timeout;
    do {
        {
            std::lock_guard<std::mutex> lock(mutex);
            if (create_calls >= count) {
                return true;
            }
        }
        std::this_thread::sleep_for(std::chrono::milliseconds(1));
    } while (std::chrono::steady_clock::now() < deadline);

    std::lock_guard<std::mutex> lock(mutex);
    return create_calls >= count;
}

MockFactoryControl& mockFactoryControl() {
    static MockFactoryControl control;
    return control;
}

void installMockTransportFactory() {
    detail::setTransportFactoryForTesting(
        [](uint32_t domain_id,
           const ConnectionOptions& options,
           detail::TransportHandlers handlers,
           detail::TransportTerminalObserver terminal_observer) {
            auto session = std::make_shared<MockSession>();
            session->domain_id = domain_id;
            session->options = options;
            session->handlers = std::move(handlers);
            session->terminal_observer = std::move(terminal_observer);

            auto& control = mockFactoryControl();
            bool fail_create = false;
            bool throw_nonstandard = false;
            bool inject_motion = false;
            MotionState motion_sample;
            {
                std::unique_lock<std::mutex> lock(control.mutex);
                initializeMatchStatus(
                    *session, control.match_on_create);
                control.latest_session = session;
                ++control.create_calls;
                control.changed.notify_all();
                control.changed.wait(lock, [&] {
                    return !control.block_create ||
                           control.release_create;
                });
                fail_create = control.fail_create;
                throw_nonstandard = control.throw_nonstandard_create;
                inject_motion = control.inject_motion_during_create;
                motion_sample = control.create_motion_sample;
            }

            if (inject_motion) {
                session->injectMotionState(std::move(motion_sample));
            }
            if (throw_nonstandard) {
                throw 9;
            }
            if (fail_create) {
                throw ConnectionError("injected startup failure");
            }
            return std::make_unique<MockTransport>(
                std::move(session));
        });
}

void resetMockTransportFixture() {
    mockFactoryControl().reset();
    installMockTransportFactory();
}

}  // namespace fourier::aurora_sdk::testing
