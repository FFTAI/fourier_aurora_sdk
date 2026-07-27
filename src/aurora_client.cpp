#include "fourier_aurora_sdk/aurora_client.hpp"

#include <algorithm>
#include <chrono>
#include <condition_variable>
#include <exception>
#include <iostream>
#include <memory>
#include <mutex>
#include <type_traits>
#include <utility>

#include "internal/callback_context.hpp"
#include "internal/connection_options.hpp"
#include "internal/teardown_coordinator.hpp"
#include "internal/transport.hpp"

namespace fourier::aurora_sdk {

namespace {

template <typename Entry>
std::vector<double> selectJointField(const Entry& entry, JointField field) {
    switch (field) {
        case JointField::Position:
            return entry.joint_position;
        case JointField::Velocity:
            return entry.joint_velocity;
        case JointField::Effort:
            return entry.joint_effort;
    }
    throw InvalidArgumentError("unsupported JointField");
}

OperationResult makeResult(uint32_t code, const std::string& detail = {}) {
    OperationResult result{
        code,
        std::string(defaultResultMessage(code)),
    };
    if (!detail.empty()) {
        result.message += ": ";
        result.message += detail;
    }
    return result;
}

bool endpointEnabled(
    const ConnectionOptions& options,
    Endpoint endpoint) {
    return std::find(
               options.enabled_endpoints.begin(),
               options.enabled_endpoints.end(),
               endpoint) != options.enabled_endpoints.end();
}

EndpointStatus& endpointStatus(
    EndpointMatchStatus& status,
    Endpoint endpoint) {
    switch (endpoint) {
        case Endpoint::FsmService:
            return status.fsm_service;
        case Endpoint::MotionCommandPublisher:
            return status.motion_command_publisher;
        case Endpoint::ControlGroupCommandPublisher:
            return status.control_group_command_publisher;
        case Endpoint::SubControllerCommandPublisher:
            return status.sub_controller_command_publisher;
        case Endpoint::AuroraStateSubscriber:
            return status.aurora_state_subscriber;
        case Endpoint::MotionStateSubscriber:
            return status.motion_state_subscriber;
        case Endpoint::ControlGroupStateSubscriber:
            return status.control_group_state_subscriber;
        case Endpoint::EstimatorDataSubscriber:
            return status.estimator_data_subscriber;
        case Endpoint::ErrorCodesSubscriber:
            return status.error_codes_subscriber;
    }
    throw InvalidArgumentError("invalid Endpoint value");
}

const EndpointStatus& endpointStatus(
    const EndpointMatchStatus& status,
    Endpoint endpoint) {
    switch (endpoint) {
        case Endpoint::FsmService:
            return status.fsm_service;
        case Endpoint::MotionCommandPublisher:
            return status.motion_command_publisher;
        case Endpoint::ControlGroupCommandPublisher:
            return status.control_group_command_publisher;
        case Endpoint::SubControllerCommandPublisher:
            return status.sub_controller_command_publisher;
        case Endpoint::AuroraStateSubscriber:
            return status.aurora_state_subscriber;
        case Endpoint::MotionStateSubscriber:
            return status.motion_state_subscriber;
        case Endpoint::ControlGroupStateSubscriber:
            return status.control_group_state_subscriber;
        case Endpoint::EstimatorDataSubscriber:
            return status.estimator_data_subscriber;
        case Endpoint::ErrorCodesSubscriber:
            return status.error_codes_subscriber;
    }
    throw InvalidArgumentError("invalid Endpoint value");
}

bool allEnabledEndpointsMatched(const EndpointMatchStatus& status) {
    return
        (!status.fsm_service.enabled || status.fsm_service.matched) &&
        (!status.motion_command_publisher.enabled ||
         status.motion_command_publisher.matched) &&
        (!status.control_group_command_publisher.enabled ||
         status.control_group_command_publisher.matched) &&
        (!status.sub_controller_command_publisher.enabled ||
         status.sub_controller_command_publisher.matched) &&
        (!status.aurora_state_subscriber.enabled ||
         status.aurora_state_subscriber.matched) &&
        (!status.motion_state_subscriber.enabled ||
         status.motion_state_subscriber.matched) &&
        (!status.control_group_state_subscriber.enabled ||
         status.control_group_state_subscriber.matched) &&
        (!status.estimator_data_subscriber.enabled ||
         status.estimator_data_subscriber.matched) &&
        (!status.error_codes_subscriber.enabled ||
         status.error_codes_subscriber.matched);
}

void validateJointField(JointField field) {
    switch (field) {
        case JointField::Position:
        case JointField::Velocity:
        case JointField::Effort:
            return;
    }
    throw InvalidArgumentError("unsupported JointField");
}

}  // namespace

struct AuroraClient::Impl {
    enum class TransitionDirection {
        Start,
        Stop,
    };

    struct Transition {
        explicit Transition(TransitionDirection requested_direction)
            : direction(requested_direction) {}

        void complete(std::exception_ptr failure = nullptr) {
            {
                std::lock_guard<std::mutex> lock(mutex);
                error = std::move(failure);
                done = true;
            }
            completed.notify_all();
        }

        void wait(bool rethrow_failure) {
            std::unique_lock<std::mutex> lock(mutex);
            completed.wait(lock, [this] { return done; });
            if (rethrow_failure && error) {
                std::rethrow_exception(error);
            }
        }

        TransitionDirection direction;
        std::mutex mutex;
        std::condition_variable completed;
        bool done = false;
        std::exception_ptr error;
    };

    struct CallbackOverrides {
        AuroraStateHandler aurora_state;
        MotionStateHandler motion_state;
        ControlGroupStateHandler control_group_state;
        EstimatorDataHandler estimator_data;
        ErrorCodesHandler error_codes;
    };

    class OperationAdmission {
    public:
        OperationAdmission() = default;

        OperationAdmission(
            Impl* owner,
            detail::ITransport* transport,
            uint64_t generation)
            : owner_(owner),
              transport_(transport),
              generation_(generation) {}

        OperationAdmission(OperationAdmission&& other) noexcept
            : owner_(std::exchange(other.owner_, nullptr)),
              transport_(std::exchange(other.transport_, nullptr)),
              generation_(other.generation_) {}

        OperationAdmission& operator=(OperationAdmission&& other) noexcept {
            if (this != &other) {
                release();
                owner_ = std::exchange(other.owner_, nullptr);
                transport_ = std::exchange(other.transport_, nullptr);
                generation_ = other.generation_;
            }
            return *this;
        }

        ~OperationAdmission() {
            release();
        }

        OperationAdmission(const OperationAdmission&) = delete;
        OperationAdmission& operator=(const OperationAdmission&) = delete;

        [[nodiscard]] detail::ITransport* transport() const noexcept {
            return transport_;
        }

        [[nodiscard]] uint64_t generation() const noexcept {
            return generation_;
        }

        explicit operator bool() const noexcept {
            return owner_ != nullptr;
        }

    private:
        void release() noexcept {
            if (owner_) {
                owner_->finishOperation();
                owner_ = nullptr;
                transport_ = nullptr;
            }
        }

        Impl* owner_ = nullptr;
        detail::ITransport* transport_ = nullptr;
        uint64_t generation_ = 0;
    };

    Impl() = default;

    OperationAdmission beginOperation(
        Endpoint endpoint,
        OperationResult& failure) {
        std::lock_guard<std::mutex> lock(mutex);
        const auto& configured_options =
            session_options ? *session_options : options;
        if (!endpointEnabled(configured_options, endpoint)) {
            failure = makeResult(result_codes::ENDPOINT_DISABLED);
            return {};
        }
        if (client_state != ClientState::Running || !transport) {
            failure = makeResult(result_codes::CLIENT_NOT_RUNNING);
            return {};
        }
        ++active_operations;
        return OperationAdmission(
            this, transport.get(), session_generation);
    }

    void finishOperation() noexcept {
        std::lock_guard<std::mutex> lock(mutex);
        if (active_operations != 0) {
            --active_operations;
        }
        finalizeFatalTransitionIfDrainedLocked();
        state_changed.notify_all();
    }

    void finishCallback() noexcept {
        std::lock_guard<std::mutex> lock(mutex);
        if (active_callbacks != 0) {
            --active_callbacks;
        }
        finalizeFatalTransitionIfDrainedLocked();
        state_changed.notify_all();
    }

    void updateAuroraMatchLocked(bool matched) {
        if (!aurora_match_observed) {
            aurora_match_observed = true;
            aurora_last_matched = matched;
            if (!matched) {
                ++aurora_match_generation;
            }
            return;
        }
        if (aurora_last_matched && !matched) {
            ++aurora_match_generation;
        }
        aurora_last_matched = matched;
    }

    bool endpointMatched(
        const OperationAdmission& admission,
        Endpoint endpoint) {
        const auto status =
            admission.transport()->endpointMatchStatus();
        const bool matched = endpointStatus(status, endpoint).matched;
        if (endpoint == Endpoint::AuroraStateSubscriber) {
            std::lock_guard<std::mutex> lock(mutex);
            if (client_state == ClientState::Running &&
                admission.generation() == session_generation) {
                updateAuroraMatchLocked(matched);
            }
        }
        return matched;
    }

    EndpointMatchStatus endpointMatchStatus() {
        EndpointMatchStatus snapshot;
        detail::ITransport* active_transport = nullptr;
        uint64_t generation = 0;
        {
            std::lock_guard<std::mutex> lock(mutex);
            const auto& configured_options =
                session_options ? *session_options : options;
            for (const auto endpoint :
                 configured_options.enabled_endpoints) {
                endpointStatus(snapshot, endpoint).enabled = true;
            }
            if (client_state != ClientState::Running || !transport) {
                return snapshot;
            }
            ++active_operations;
            active_transport = transport.get();
            generation = session_generation;
        }

        OperationAdmission admission(
            this, active_transport, generation);
        const auto live = active_transport->endpointMatchStatus();
        {
            std::lock_guard<std::mutex> lock(mutex);
            if (client_state != ClientState::Running ||
                generation != session_generation) {
                return snapshot;
            }
            const auto& configured_options = *session_options;
            for (const auto endpoint :
                 configured_options.enabled_endpoints) {
                endpointStatus(snapshot, endpoint).matched =
                    endpointStatus(live, endpoint).matched;
            }
            if (endpointEnabled(
                    configured_options,
                    Endpoint::AuroraStateSubscriber)) {
                updateAuroraMatchLocked(
                    snapshot.aurora_state_subscriber.matched);
            }
        }
        return snapshot;
    }

    template <typename Value>
    OperationResult readCached(
        Endpoint endpoint,
        std::optional<Value> Impl::* cache_member,
        Value& output) {
        OperationResult failure;
        auto admission = beginOperation(endpoint, failure);
        if (!admission) {
            return failure;
        }
        if (!endpointMatched(admission, endpoint)) {
            return makeResult(result_codes::ENDPOINT_UNMATCHED);
        }

        Value snapshot;
        {
            std::lock_guard<std::mutex> lock(mutex);
            const auto& cache = this->*cache_member;
            if (!cache) {
                return makeResult(result_codes::STATE_NOT_RECEIVED);
            }
            snapshot = *cache;
        }
        output = std::move(snapshot);
        return makeResult(result_codes::SUCCESS);
    }

    void clearCachesLocked() {
        aurora_state.reset();
        motion_state.reset();
        control_group_state.reset();
        estimator_data.reset();
        error_codes.reset();
        last_motion_command.reset();
        last_control_group_command.reset();
        last_sub_controller_command.reset();
        aurora_match_observed = false;
        aurora_last_matched = false;
        aurora_match_generation = 0;
        aurora_sample_generation = 0;
    }

    void finalizeFatalTransitionIfDrainedLocked() noexcept {
        const bool explicit_stop =
            transition &&
            transition->direction == TransitionDirection::Stop;
        if (!terminal_pending || explicit_stop ||
            active_operations != 0 || active_callbacks != 0) {
            return;
        }
        clearCachesLocked();
        session_options.reset();
        terminal_pending = false;
        client_state = ClientState::Stopped;
    }

    void handleTransportTerminal(uint64_t generation) noexcept {
        std::lock_guard<std::mutex> lock(mutex);
        if (generation != session_generation) {
            return;
        }
        terminal_observed = true;
        if (client_state == ClientState::Running ||
            (client_state == ClientState::Stopping && !transition)) {
            client_state = ClientState::Stopping;
            terminal_pending = true;
            finalizeFatalTransitionIfDrainedLocked();
        } else if (client_state == ClientState::Starting) {
            terminal_pending = true;
        }
        state_changed.notify_all();
    }

    template <typename Value, typename Handler>
    void handleSample(
        uint64_t generation,
        std::optional<Value> Impl::* cache_member,
        Handler callback,
        Value value,
        const char* callback_name) {
        bool invoke_callback = false;
        {
            std::lock_guard<std::mutex> lock(mutex);
            if (generation != session_generation ||
                (client_state != ClientState::Starting &&
                 client_state != ClientState::Running)) {
                return;
            }
            this->*cache_member = value;
            if constexpr (std::is_same_v<Value, AuroraState>) {
                aurora_sample_generation = aurora_match_generation;
            }
            invoke_callback =
                client_state == ClientState::Running &&
                static_cast<bool>(callback);
            if (invoke_callback) {
                ++active_callbacks;
            }
            state_changed.notify_all();
        }

        if (!invoke_callback) {
            return;
        }

        try {
            detail::UserCallbackScope callback_scope;
            callback(std::move(value));
        } catch (const std::exception& error) {
            std::cerr << "[fourier_aurora_sdk] " << callback_name
                      << " callback failed: " << error.what() << std::endl;
        } catch (...) {
            std::cerr << "[fourier_aurora_sdk] " << callback_name
                      << " callback failed with a non-standard exception"
                      << std::endl;
        }
        finishCallback();
    }

    detail::TransportHandlers makeHandlers(
        uint64_t generation,
        const CallbackOverrides& callbacks) {
        detail::TransportHandlers handlers;
        handlers.aurora_state =
            [this, generation, callback = callbacks.aurora_state](
                AuroraState state) mutable {
                handleSample(
                    generation,
                    &Impl::aurora_state,
                    callback,
                    std::move(state),
                    "AuroraState");
            };
        handlers.motion_state =
            [this, generation, callback = callbacks.motion_state](
                MotionState state) mutable {
                handleSample(
                    generation,
                    &Impl::motion_state,
                    callback,
                    std::move(state),
                    "MotionState");
            };
        handlers.control_group_state =
            [this, generation, callback = callbacks.control_group_state](
                ControlGroupState state) mutable {
                handleSample(
                    generation,
                    &Impl::control_group_state,
                    callback,
                    std::move(state),
                    "ControlGroupState");
            };
        handlers.estimator_data =
            [this, generation, callback = callbacks.estimator_data](
                EstimatorData data) mutable {
                handleSample(
                    generation,
                    &Impl::estimator_data,
                    callback,
                    std::move(data),
                    "EstimatorData");
            };
        handlers.error_codes =
            [this, generation, callback = callbacks.error_codes](
                ErrorCodes codes) mutable {
                handleSample(
                    generation,
                    &Impl::error_codes,
                    callback,
                    std::move(codes),
                    "ErrorCodes");
            };
        return handlers;
    }

    void start() {
        if (detail::inUserCallback()) {
            std::lock_guard<std::mutex> lock(mutex);
            if (client_state == ClientState::Running) {
                return;
            }
            throw InvalidStateError(
                "start() cannot wait or transition from an SDK callback");
        }

        for (;;) {
            std::shared_ptr<Transition> joined_transition;
            bool same_direction = false;
            std::shared_ptr<Transition> owned_transition;
            ConnectionOptions frozen_options;
            CallbackOverrides frozen_callbacks;
            uint32_t frozen_domain_id = 0;
            uint64_t generation = 0;
            std::unique_ptr<detail::ITransport> retired_transport;

            {
                std::unique_lock<std::mutex> lock(mutex);
                if (client_state == ClientState::Running) {
                    return;
                }
                if (transition) {
                    joined_transition = transition;
                    same_direction =
                        transition->direction == TransitionDirection::Start;
                } else if (client_state == ClientState::Stopping) {
                    state_changed.wait(lock, [this] {
                        return client_state != ClientState::Stopping ||
                               static_cast<bool>(transition);
                    });
                    continue;
                } else {
                    retired_transport = std::move(transport);
                    client_state = ClientState::Starting;
                    owned_transition =
                        std::make_shared<Transition>(
                            TransitionDirection::Start);
                    transition = owned_transition;
                    frozen_options = options;
                    frozen_callbacks = callback_overrides;
                    frozen_domain_id = domain_id;
                    session_options = frozen_options;
                    generation = ++session_generation;
                    terminal_observed = false;
                    terminal_pending = false;
                    clearCachesLocked();
                    state_changed.notify_all();
                }
            }

            if (joined_transition) {
                joined_transition->wait(same_direction);
                if (same_direction) {
                    return;
                }
                continue;
            }

            std::unique_ptr<detail::ITransport> new_transport;
            std::exception_ptr startup_error;
            try {
                if (retired_transport) {
                    retired_transport->stop();
                    retired_transport.reset();
                }
                auto handlers = makeHandlers(generation, frozen_callbacks);
                new_transport = detail::makeProductionTransport(
                    frozen_domain_id,
                    frozen_options,
                    std::move(handlers),
                    [this, generation] {
                        handleTransportTerminal(generation);
                    });
            } catch (...) {
                startup_error = std::current_exception();
            }

            {
                std::lock_guard<std::mutex> lock(mutex);
                if (!startup_error && !terminal_pending) {
                    transport = std::move(new_transport);
                    client_state = ClientState::Running;
                } else {
                    if (!startup_error) {
                        startup_error = std::make_exception_ptr(
                            ConnectionError(
                                "transport stopped during startup"));
                    }
                    clearCachesLocked();
                    session_options.reset();
                    terminal_pending = false;
                    client_state = ClientState::Stopped;
                }
                if (transition == owned_transition) {
                    transition.reset();
                }
                state_changed.notify_all();
            }

            owned_transition->complete(startup_error);
            if (startup_error) {
                try {
                    std::rethrow_exception(startup_error);
                } catch (const std::exception& error) {
                    std::cerr
                        << "[fourier_aurora_sdk] client startup failed: "
                        << error.what() << std::endl;
                    throw;
                } catch (...) {
                    std::cerr
                        << "[fourier_aurora_sdk] client startup failed"
                        << std::endl;
                    throw;
                }
            }
            return;
        }
    }

    void stop() {
        if (detail::inUserCallback()) {
            throw InvalidStateError(
                "stop() must not be called from an SDK callback");
        }

        for (;;) {
            std::shared_ptr<Transition> joined_transition;
            bool same_direction = false;
            std::shared_ptr<Transition> owned_transition;
            std::unique_ptr<detail::ITransport> detached_transport;

            {
                std::unique_lock<std::mutex> lock(mutex);
                if (transition) {
                    joined_transition = transition;
                    same_direction =
                        transition->direction == TransitionDirection::Stop;
                } else if (client_state == ClientState::Stopping) {
                    state_changed.wait(lock, [this] {
                        return client_state != ClientState::Stopping ||
                               static_cast<bool>(transition);
                    });
                    continue;
                } else if (
                    client_state == ClientState::Stopped && !transport) {
                    return;
                } else {
                    client_state = ClientState::Stopping;
                    owned_transition =
                        std::make_shared<Transition>(
                            TransitionDirection::Stop);
                    transition = owned_transition;
                    state_changed.notify_all();
                    state_changed.wait(lock, [this] {
                        return active_operations == 0 &&
                               active_callbacks == 0;
                    });
                    detached_transport = std::move(transport);
                }
            }

            if (joined_transition) {
                joined_transition->wait(same_direction);
                if (same_direction) {
                    return;
                }
                continue;
            }

            std::exception_ptr teardown_error;
            if (detached_transport) {
                try {
                    detached_transport->stop();
                } catch (...) {
                    teardown_error = std::current_exception();
                }
                detached_transport.reset();
            }

            {
                std::lock_guard<std::mutex> lock(mutex);
                clearCachesLocked();
                session_options.reset();
                terminal_pending = false;
                client_state = ClientState::Stopped;
                if (transition == owned_transition) {
                    transition.reset();
                }
                state_changed.notify_all();
            }
            owned_transition->complete(teardown_error);

            if (teardown_error) {
                try {
                    std::rethrow_exception(teardown_error);
                } catch (const std::exception& error) {
                    std::cerr
                        << "[fourier_aurora_sdk] client teardown failed: "
                        << error.what() << std::endl;
                    throw;
                } catch (...) {
                    std::cerr
                        << "[fourier_aurora_sdk] client teardown failed"
                        << std::endl;
                    throw;
                }
            }
            return;
        }
    }

    mutable std::mutex mutex;
    std::condition_variable state_changed;
    ClientState client_state = ClientState::Stopped;
    std::shared_ptr<Transition> transition;

    std::string user_id = "aurora_client";
    uint32_t domain_id = 123;
    ConnectionOptions options;
    CallbackOverrides callback_overrides;

    std::optional<ConnectionOptions> session_options;
    uint64_t session_generation = 0;
    bool terminal_observed = false;
    bool terminal_pending = false;
    std::unique_ptr<detail::ITransport> transport;

    std::size_t active_operations = 0;
    std::size_t active_callbacks = 0;

    std::optional<AuroraState> aurora_state;
    std::optional<MotionState> motion_state;
    std::optional<ControlGroupState> control_group_state;
    std::optional<EstimatorData> estimator_data;
    std::optional<ErrorCodes> error_codes;
    std::optional<MotionCommand> last_motion_command;
    std::optional<ControlGroupCommand> last_control_group_command;
    std::optional<SubControllerCommand> last_sub_controller_command;
    bool aurora_match_observed = false;
    bool aurora_last_matched = false;
    uint64_t aurora_match_generation = 0;
    uint64_t aurora_sample_generation = 0;
};

// Returns the process-wide client and ensures fatal teardown support exists.
AuroraClient& AuroraClient::getInstance() {
    (void)detail::TeardownCoordinator::instance();
    static AuroraClient instance;
    return instance;
}

// Initializes the singleton in its stopped, configurable state.
AuroraClient::AuroraClient() : impl_(std::make_unique<Impl>()) {}

// Provides no-throw fallback cleanup when the application omitted stop().
AuroraClient::~AuroraClient() noexcept {
    try {
        impl_->stop();
    } catch (const std::exception& error) {
        std::cerr << "[fourier_aurora_sdk] client destructor cleanup failed: "
                  << error.what() << std::endl;
    } catch (...) {
        std::cerr << "[fourier_aurora_sdk] client destructor cleanup failed"
                  << std::endl;
    }
}

// Returns a copy so callers cannot mutate the canonical options implicitly.
ConnectionOptions AuroraClient::config() const {
    std::lock_guard<std::mutex> lock(impl_->mutex);
    return impl_->options;
}

// Validates and replaces the complete stopped-stage connection configuration.
void AuroraClient::configure(const ConnectionOptions& options) {
    const auto canonical =
        detail::normalizeConnectionOptions(options).public_options;
    std::lock_guard<std::mutex> lock(impl_->mutex);
    if (impl_->client_state != ClientState::Stopped) {
        throw InvalidStateError(
            "configure() is allowed only while the client is stopped");
    }
    impl_->options = canonical;
}

// Selects the DDS domain for subsequent transport sessions.
void AuroraClient::setDomainId(uint32_t domain_id) {
    std::lock_guard<std::mutex> lock(impl_->mutex);
    if (impl_->client_state != ClientState::Stopped) {
        throw InvalidStateError(
            "setDomainId() is allowed only while the client is stopped");
    }
    impl_->domain_id = domain_id;
}

// Validates and stores the SDK-local diagnostic label for future sessions.
void AuroraClient::setUserId(std::string user_id) {
    detail::validateUserId(user_id);
    std::lock_guard<std::mutex> lock(impl_->mutex);
    if (impl_->client_state != ClientState::Stopped) {
        throw InvalidStateError(
            "setUserId() is allowed only while the client is stopped");
    }
    impl_->user_id = std::move(user_id);
}

// Reads the configured DDS domain independently of transport state.
uint32_t AuroraClient::domainId() const {
    std::lock_guard<std::mutex> lock(impl_->mutex);
    return impl_->domain_id;
}

// Reads the configured SDK-local diagnostic label.
std::string AuroraClient::userId() const {
    std::lock_guard<std::mutex> lock(impl_->mutex);
    return impl_->user_id;
}

// Reports only the local lifecycle state, not endpoint or robot readiness.
ClientState AuroraClient::state() const {
    std::lock_guard<std::mutex> lock(impl_->mutex);
    return impl_->client_state;
}

// Creates a transport session from a frozen copy of the current configuration.
void AuroraClient::start() {
    impl_->start();
}

// Synchronously drains callbacks and operations, then destroys transport resources.
void AuroraClient::stop() {
    impl_->stop();
}

// Takes one enabled/matched snapshot across the configured endpoint set.
EndpointMatchStatus AuroraClient::endpointMatchStatus() const {
    return impl_->endpointMatchStatus();
}

// Provides the common single-endpoint readiness check for the FSM service.
bool AuroraClient::isFsmServiceMatched() const {
    return endpointMatchStatus().fsm_service.matched;
}

// Polls local match state until every enabled endpoint is ready or time expires.
bool AuroraClient::waitForEndpoints(std::chrono::milliseconds timeout) {
    if (timeout.count() < 0) {
        throw InvalidArgumentError("timeout must be nonnegative");
    }
    const auto start = std::chrono::steady_clock::now();
    const auto available =
        std::chrono::steady_clock::time_point::max() - start;
    const auto deadline =
        timeout >= available
            ? std::chrono::steady_clock::time_point::max()
            : start + timeout;

    for (;;) {
        const auto status = endpointMatchStatus();
        {
            std::lock_guard<std::mutex> lock(impl_->mutex);
            if (impl_->client_state != ClientState::Running) {
                return false;
            }
        }
        if (allEnabledEndpointsMatched(status)) {
            return true;
        }

        const auto now = std::chrono::steady_clock::now();
        if (now >= deadline) {
            return false;
        }
        const auto interval = std::min<std::chrono::steady_clock::duration>(
            std::chrono::milliseconds(10), deadline - now);
        std::unique_lock<std::mutex> lock(impl_->mutex);
        impl_->state_changed.wait_for(lock, interval);
    }
}

// Waits for a fresh Aurora snapshot whose current FSM ID matches the request.
bool AuroraClient::waitForFsmState(
    uint32_t state_id,
    std::chrono::milliseconds timeout) {
    if (timeout.count() < 0) {
        throw InvalidArgumentError("timeout must be nonnegative");
    }
    const auto start = std::chrono::steady_clock::now();
    const auto available =
        std::chrono::steady_clock::time_point::max() - start;
    const auto deadline =
        timeout >= available
            ? std::chrono::steady_clock::time_point::max()
            : start + timeout;

    for (;;) {
        bool state_matches = false;
        {
            OperationResult failure;
            auto admission = impl_->beginOperation(
                Endpoint::AuroraStateSubscriber, failure);
            if (!admission) {
                return false;
            }
            if (impl_->endpointMatched(
                    admission, Endpoint::AuroraStateSubscriber)) {
                std::lock_guard<std::mutex> lock(impl_->mutex);
                state_matches =
                    impl_->client_state == ClientState::Running &&
                    admission.generation() == impl_->session_generation &&
                    impl_->aurora_state &&
                    impl_->aurora_state->current_state.id == state_id &&
                    impl_->aurora_sample_generation ==
                        impl_->aurora_match_generation;
            }
        }
        if (state_matches) {
            return true;
        }

        const auto now = std::chrono::steady_clock::now();
        if (now >= deadline) {
            return false;
        }
        std::unique_lock<std::mutex> lock(impl_->mutex);
        if (impl_->client_state != ClientState::Running) {
            return false;
        }
        const auto interval = std::min<std::chrono::steady_clock::duration>(
            std::chrono::milliseconds(10), deadline - now);
        impl_->state_changed.wait_for(lock, interval);
    }
}

// Copies the latest complete Aurora-state snapshot into the caller's output.
OperationResult AuroraClient::getAuroraState(AuroraState& output) const {
    return impl_->readCached(
        Endpoint::AuroraStateSubscriber,
        &Impl::aurora_state,
        output);
}

// Copies the latest complete motion-state snapshot into the caller's output.
OperationResult AuroraClient::getMotionState(MotionState& output) const {
    return impl_->readCached(
        Endpoint::MotionStateSubscriber,
        &Impl::motion_state,
        output);
}

// Copies the latest complete control-group snapshot into the caller's output.
OperationResult AuroraClient::getControlGroupState(
    ControlGroupState& output) const {
    return impl_->readCached(
        Endpoint::ControlGroupStateSubscriber,
        &Impl::control_group_state,
        output);
}

// Copies the latest complete estimator snapshot into the caller's output.
OperationResult AuroraClient::getEstimatorData(
    EstimatorData& output) const {
    return impl_->readCached(
        Endpoint::EstimatorDataSubscriber,
        &Impl::estimator_data,
        output);
}

// Copies the latest asynchronous robot error-code snapshot.
OperationResult AuroraClient::getErrorCodes(ErrorCodes& output) const {
    return impl_->readCached(
        Endpoint::ErrorCodesSubscriber,
        &Impl::error_codes,
        output);
}

// Projects base pose and motion fields from one estimator snapshot.
OperationResult AuroraClient::getBaseData(BaseData& output) const {
    EstimatorData estimator;
    const auto result = impl_->readCached(
        Endpoint::EstimatorDataSubscriber,
        &Impl::estimator_data,
        estimator);
    if (!result.success()) {
        return result;
    }

    BaseData value;
    value.header = estimator.header;
    value.quat_xyzw = estimator.quat_xyzw;
    value.quat_wxyz = estimator.quat_wxyz;
    value.rpy = estimator.rpy;
    value.omega_W = estimator.omega_W;
    value.acc_W = estimator.acc_W;
    value.omega_B = estimator.omega_B;
    value.acc_B = estimator.acc_B;
    value.vel_W = estimator.vel_W;
    value.pos_W = estimator.pos_W;
    value.vel_B = estimator.vel_B;
    value.stable_level = estimator.stable_level;
    output = std::move(value);
    return makeResult(result_codes::SUCCESS);
}

// Projects contact forces and probabilities from one estimator snapshot.
OperationResult AuroraClient::getContactData(ContactData& output) const {
    EstimatorData estimator;
    const auto result = impl_->readCached(
        Endpoint::EstimatorDataSubscriber,
        &Impl::estimator_data,
        estimator);
    if (!result.success()) {
        return result;
    }

    ContactData value;
    value.header = estimator.header;
    value.contact_force = estimator.contact_force;
    value.contact_prob = estimator.contact_prob;
    output = std::move(value);
    return makeResult(result_codes::SUCCESS);
}

// Extracts stable level from one motion-state snapshot.
OperationResult AuroraClient::getStableLevel(uint8_t& output) const {
    MotionState motion;
    const auto result = impl_->readCached(
        Endpoint::MotionStateSubscriber,
        &Impl::motion_state,
        motion);
    if (!result.success()) {
        return result;
    }
    output = motion.stable_level;
    return makeResult(result_codes::SUCCESS);
}

// Builds the compact height, pitch, yaw, and stable-level stand-pose view.
OperationResult AuroraClient::getStandPose(
    std::vector<double>& output) const {
    MotionState motion;
    const auto result = impl_->readCached(
        Endpoint::MotionStateSubscriber,
        &Impl::motion_state,
        motion);
    if (!result.success()) {
        return result;
    }
    output = {
        motion.delta_height_cmd,
        motion.attitude_cmd[1],
        motion.attitude_cmd[2],
        static_cast<double>(motion.stable_level),
    };
    return makeResult(result_codes::SUCCESS);
}

// Finds a named group and returns one measured joint field from one snapshot.
OperationResult AuroraClient::getJointState(
    const std::string& group_name,
    JointField field,
    std::vector<double>& output) const {
    validateJointField(field);
    ControlGroupState state;
    const auto result = impl_->readCached(
        Endpoint::ControlGroupStateSubscriber,
        &Impl::control_group_state,
        state);
    if (!result.success()) {
        return result;
    }
    const auto it = std::find_if(
        state.joint_state.begin(),
        state.joint_state.end(),
        [&](const ControlGroupStateEntry& entry) {
            return entry.group_name == group_name;
        });
    if (it == state.joint_state.end()) {
        return makeResult(
            result_codes::CLIENT_ERROR,
            "joint state group not found: " + group_name);
    }
    output = selectJointField(*it, field);
    return makeResult(result_codes::SUCCESS);
}

// Finds a named group and returns one commanded joint field from one snapshot.
OperationResult AuroraClient::getJointCommand(
    const std::string& group_name,
    JointField field,
    std::vector<double>& output) const {
    validateJointField(field);
    ControlGroupState state;
    const auto result = impl_->readCached(
        Endpoint::ControlGroupStateSubscriber,
        &Impl::control_group_state,
        state);
    if (!result.success()) {
        return result;
    }
    const auto it = std::find_if(
        state.joint_cmd.begin(),
        state.joint_cmd.end(),
        [&](const ControlGroupStateEntry& entry) {
            return entry.group_name == group_name;
        });
    if (it == state.joint_cmd.end()) {
        return makeResult(
            result_codes::CLIENT_ERROR,
            "joint command group not found: " + group_name);
    }
    output = selectJointField(*it, field);
    return makeResult(result_codes::SUCCESS);
}

// Finds and returns one named link's measured Cartesian state.
OperationResult AuroraClient::getCartesianState(
    const std::string& link_name,
    CartesianStateEntry& output) const {
    ControlGroupState state;
    const auto result = impl_->readCached(
        Endpoint::ControlGroupStateSubscriber,
        &Impl::control_group_state,
        state);
    if (!result.success()) {
        return result;
    }
    const auto it = std::find_if(
        state.cartesian_state.begin(),
        state.cartesian_state.end(),
        [&](const CartesianStateEntry& entry) {
            return entry.link_name == link_name;
        });
    if (it == state.cartesian_state.end()) {
        return makeResult(
            result_codes::CLIENT_ERROR,
            "cartesian state link not found: " + link_name);
    }
    output = *it;
    return makeResult(result_codes::SUCCESS);
}

// Finds and returns one named link's observed Cartesian command.
OperationResult AuroraClient::getCartesianCommand(
    const std::string& link_name,
    CartesianStateEntry& output) const {
    ControlGroupState state;
    const auto result = impl_->readCached(
        Endpoint::ControlGroupStateSubscriber,
        &Impl::control_group_state,
        state);
    if (!result.success()) {
        return result;
    }
    const auto it = std::find_if(
        state.cartesian_cmd.begin(),
        state.cartesian_cmd.end(),
        [&](const CartesianStateEntry& entry) {
            return entry.link_name == link_name;
        });
    if (it == state.cartesian_cmd.end()) {
        return makeResult(
            result_codes::CLIENT_ERROR,
            "cartesian command link not found: " + link_name);
    }
    output = *it;
    return makeResult(result_codes::SUCCESS);
}

// Returns the last motion command successfully transmitted by this client.
OperationResult AuroraClient::getLastMotionCommand(
    MotionCommand& output) const {
    return impl_->readCached(
        Endpoint::MotionCommandPublisher,
        &Impl::last_motion_command,
        output);
}

// Returns the last control-group command successfully transmitted by this client.
OperationResult AuroraClient::getLastControlGroupCommand(
    ControlGroupCommand& output) const {
    return impl_->readCached(
        Endpoint::ControlGroupCommandPublisher,
        &Impl::last_control_group_command,
        output);
}

// Returns the last sub-controller command successfully transmitted by this client.
OperationResult AuroraClient::getLastSubControllerCommand(
    SubControllerCommand& output) const {
    return impl_->readCached(
        Endpoint::SubControllerCommandPublisher,
        &Impl::last_sub_controller_command,
        output);
}

// Stamps, publishes, and caches one complete motion command.
OperationResult AuroraClient::publishMotionCommand(
    const MotionCommand& command) {
    OperationResult failure;
    auto admission = impl_->beginOperation(
        Endpoint::MotionCommandPublisher, failure);
    if (!admission) {
        return failure;
    }

    MotionCommand transmitted = command;
    transmitted.header.send_time = std::chrono::system_clock::now();
    transmitted.header.frame_id.clear();
    transmitted.header.received_time = {};
    const auto result =
        admission.transport()->publishMotionCommand(transmitted);
    if (result.success()) {
        std::lock_guard<std::mutex> lock(impl_->mutex);
        impl_->last_motion_command = transmitted;
        impl_->state_changed.notify_all();
    }
    return result;
}

// Stamps, publishes, and caches one complete control-group command.
OperationResult AuroraClient::publishControlGroupCommand(
    const ControlGroupCommand& command) {
    OperationResult failure;
    auto admission = impl_->beginOperation(
        Endpoint::ControlGroupCommandPublisher, failure);
    if (!admission) {
        return failure;
    }

    ControlGroupCommand transmitted = command;
    transmitted.header.send_time = std::chrono::system_clock::now();
    transmitted.header.frame_id.clear();
    transmitted.header.received_time = {};
    const auto result =
        admission.transport()->publishControlGroupCommand(transmitted);
    if (result.success()) {
        std::lock_guard<std::mutex> lock(impl_->mutex);
        impl_->last_control_group_command = transmitted;
        impl_->state_changed.notify_all();
    }
    return result;
}

// Stamps, publishes, and caches one complete sub-controller command.
OperationResult AuroraClient::publishSubControllerCommand(
    const SubControllerCommand& command) {
    OperationResult failure;
    auto admission = impl_->beginOperation(
        Endpoint::SubControllerCommandPublisher, failure);
    if (!admission) {
        return failure;
    }

    SubControllerCommand transmitted = command;
    transmitted.header.send_time = std::chrono::system_clock::now();
    transmitted.header.frame_id.clear();
    transmitted.header.received_time = {};
    const auto result =
        admission.transport()->publishSubControllerCommand(transmitted);
    if (result.success()) {
        std::lock_guard<std::mutex> lock(impl_->mutex);
        impl_->last_sub_controller_command = transmitted;
        impl_->state_changed.notify_all();
    }
    return result;
}

// Sends one typed FSM request and preserves Aurora's returned result.
OperationResult AuroraClient::requestFsmState(uint32_t state_id) {
    OperationResult failure;
    auto admission =
        impl_->beginOperation(Endpoint::FsmService, failure);
    if (!admission) {
        return failure;
    }
    return admission.transport()->requestFsmState(state_id);
}

// Converts user-facing base-frame velocities into a complete motion command.
OperationResult AuroraClient::setVelocity(
    double vx,
    double vy,
    double yaw,
    double duration,
    uint8_t gait_type) {
    MotionCommand command;
    command.lin_velocity = {vx, vy, 0.0};
    command.ang_velocity = {0.0, 0.0, yaw};
    command.duration = duration;
    command.gait_type = gait_type;
    return publishMotionCommand(command);
}

// Converts nominal-standing pose offsets into a complete motion command.
OperationResult AuroraClient::setStandPose(
    double delta_height,
    double delta_pitch,
    double delta_yaw,
    double duration) {
    MotionCommand command;
    command.delta_height = delta_height;
    command.attitude = {0.0, delta_pitch, delta_yaw};
    command.duration = duration;
    return publishMotionCommand(command);
}

// Converts a reset request into a complete sub-controller command.
OperationResult AuroraClient::resetUnicontroller() {
    SubControllerCommand command;
    command.reset = true;
    return publishSubControllerCommand(command);
}

// Stores the stopped-stage callback invoked after Aurora-state cache updates.
void AuroraClient::onAuroraState(AuroraStateHandler callback) {
    std::lock_guard<std::mutex> lock(impl_->mutex);
    if (impl_->client_state != ClientState::Stopped) {
        throw InvalidStateError(
            "callback overrides can change only while the client is stopped");
    }
    impl_->callback_overrides.aurora_state = std::move(callback);
}

// Stores the stopped-stage callback invoked after motion-state cache updates.
void AuroraClient::onMotionState(MotionStateHandler callback) {
    std::lock_guard<std::mutex> lock(impl_->mutex);
    if (impl_->client_state != ClientState::Stopped) {
        throw InvalidStateError(
            "callback overrides can change only while the client is stopped");
    }
    impl_->callback_overrides.motion_state = std::move(callback);
}

// Stores the stopped-stage callback invoked after control-group cache updates.
void AuroraClient::onControlGroupState(
    ControlGroupStateHandler callback) {
    std::lock_guard<std::mutex> lock(impl_->mutex);
    if (impl_->client_state != ClientState::Stopped) {
        throw InvalidStateError(
            "callback overrides can change only while the client is stopped");
    }
    impl_->callback_overrides.control_group_state = std::move(callback);
}

// Stores the stopped-stage callback invoked after estimator cache updates.
void AuroraClient::onEstimatorData(EstimatorDataHandler callback) {
    std::lock_guard<std::mutex> lock(impl_->mutex);
    if (impl_->client_state != ClientState::Stopped) {
        throw InvalidStateError(
            "callback overrides can change only while the client is stopped");
    }
    impl_->callback_overrides.estimator_data = std::move(callback);
}

// Stores the stopped-stage callback invoked after error-code cache updates.
void AuroraClient::onErrorCodes(ErrorCodesHandler callback) {
    std::lock_guard<std::mutex> lock(impl_->mutex);
    if (impl_->client_state != ClientState::Stopped) {
        throw InvalidStateError(
            "callback overrides can change only while the client is stopped");
    }
    impl_->callback_overrides.error_codes = std::move(callback);
}

}  // namespace fourier::aurora_sdk
