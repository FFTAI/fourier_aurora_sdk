#include "fourier_aurora_sdk/aurora_client.hpp"

#include <algorithm>
#include <array>
#include <chrono>
#include <cmath>
#include <condition_variable>
#include <exception>
#include <iostream>
#include <limits>
#include <memory>
#include <mutex>
#include <random>
#include <set>
#include <sstream>
#include <string>
#include <thread>
#include <type_traits>
#include <utility>

#include "internal/callback_context.hpp"
#include "internal/connection_options.hpp"
#include "internal/teardown_coordinator.hpp"
#include "internal/transport.hpp"

namespace fourier::aurora_sdk {

namespace {

constexpr auto kEndpointMatchStabilizationDelay =
    std::chrono::milliseconds(200);
constexpr auto kEndpointMatchProgressPeriod = std::chrono::seconds(1);

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

const EndpointStatus& endpointStatus(
    const EndpointMatchStatus& status,
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

struct EndpointDescription {
    Endpoint endpoint;
    const char* name;
};

constexpr std::array<EndpointDescription, 12> kEndpointDescriptions{{
    {Endpoint::FsmService, "FsmService"},
    {Endpoint::ManageLeaseService, "ManageLeaseService"},
    {Endpoint::MotionCommandPublisher, "MotionCommandPublisher"},
    {Endpoint::ControlGroupCommandPublisher,
     "ControlGroupCommandPublisher"},
    {Endpoint::MotorConfigurationCommandPublisher,
     "MotorConfigurationCommandPublisher"},
    {Endpoint::SubControllerService, "SubControllerService"},
    {Endpoint::AuroraStateSubscriber, "AuroraStateSubscriber"},
    {Endpoint::MotionStateSubscriber, "MotionStateSubscriber"},
    {Endpoint::ControlGroupStateSubscriber,
     "ControlGroupStateSubscriber"},
    {Endpoint::MotorConfigurationStateSubscriber,
     "MotorConfigurationStateSubscriber"},
    {Endpoint::EstimatorDataSubscriber, "EstimatorDataSubscriber"},
    {Endpoint::ErrorCodesSubscriber, "ErrorCodesSubscriber"},
}};

std::string formatEndpointsByMatchState(
    const EndpointMatchStatus& status,
    bool matched) {
    std::ostringstream output;
    output << "[";
    bool first = true;
    for (const auto& description : kEndpointDescriptions) {
        const auto& endpoint = endpointStatus(status, description.endpoint);
        if (!endpoint.enabled || endpoint.matched != matched) {
            continue;
        }
        if (!first) {
            output << ", ";
        }
        first = false;
        output << description.name;
    }
    output << "]";
    return output.str();
}

std::string formatEndpointMatchSummary(
    const EndpointMatchStatus& status) {
    return "matched: " + formatEndpointsByMatchState(status, true) +
           ", unmatched: " + formatEndpointsByMatchState(status, false);
}

bool allEnabledEndpointsMatched(const EndpointMatchStatus& status) {
    return
        (!status.fsm_service.enabled || status.fsm_service.matched) &&
        (!status.manage_lease_service.enabled || status.manage_lease_service.matched) &&
        (!status.motion_command_publisher.enabled ||
         status.motion_command_publisher.matched) &&
        (!status.control_group_command_publisher.enabled ||
         status.control_group_command_publisher.matched) &&
        (!status.motor_configuration_command_publisher.enabled ||
         status.motor_configuration_command_publisher.matched) &&
        (!status.sub_controller_service.enabled || status.sub_controller_service.matched) &&
        (!status.aurora_state_subscriber.enabled ||
         status.aurora_state_subscriber.matched) &&
        (!status.motion_state_subscriber.enabled ||
         status.motion_state_subscriber.matched) &&
        (!status.control_group_state_subscriber.enabled ||
         status.control_group_state_subscriber.matched) &&
        (!status.motor_configuration_state_subscriber.enabled ||
         status.motor_configuration_state_subscriber.matched) &&
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

const std::array<const char*, 9> kResourceNames{
    "left_leg", "right_leg", "waist", "head", "left_manipulator",
    "right_manipulator", "left_end_effector", "right_end_effector", "motion_cmd"};

std::size_t resourceIndex(AuroraResource resource) {
    const auto index = static_cast<std::size_t>(resource);
    if (index >= kResourceNames.size()) {
        throw InvalidArgumentError("invalid AuroraResource value");
    }
    return index;
}

std::vector<AuroraResource> canonicalResources(
    const std::vector<AuroraResource>& resources) {
    if (resources.empty()) {
        throw InvalidArgumentError("lease resource set must not be empty");
    }
    std::array<bool, 9> seen{};
    for (const auto resource : resources) {
        const auto index = resourceIndex(resource);
        if (seen[index]) {
            throw InvalidArgumentError("lease resource set contains a duplicate");
        }
        seen[index] = true;
    }
    std::vector<AuroraResource> canonical;
    canonical.reserve(resources.size());
    for (std::size_t index = 0; index < seen.size(); ++index) {
        if (seen[index]) {
            canonical.push_back(static_cast<AuroraResource>(index));
        }
    }
    return canonical;
}

std::vector<std::string> resourceNames(
    const std::vector<AuroraResource>& resources) {
    std::vector<std::string> names;
    names.reserve(resources.size());
    for (const auto resource : resources) {
        names.emplace_back(kResourceNames[resourceIndex(resource)]);
    }
    return names;
}

std::optional<AuroraResource> groupResource(const std::string& group) {
    for (std::size_t index = 0; index < 8; ++index) {
        if (group == kResourceNames[index]) {
            return static_cast<AuroraResource>(index);
        }
    }
    return std::nullopt;
}

void validateLeaseOption(const LeaseOption& option) {
    if (option.duration < std::chrono::milliseconds{100}) {
        throw InvalidArgumentError("lease duration must be at least 100 ms");
    }
    if (option.timeout <= std::chrono::milliseconds{0}) {
        throw InvalidArgumentError("lease operation timeout must be positive");
    }
    if (option.priority.has_value() &&
        *option.priority != LeasePriority::Normal &&
        *option.priority != LeasePriority::Emergency) {
        throw InvalidArgumentError("invalid LeasePriority value");
    }
}

uint32_t mapLeaseWireResult(uint32_t wire_result) {
    switch (wire_result) {
        case 0: return result_codes::SUCCESS;
        case 101: return result_codes::LEASE_INVALID_REQUEST;
        case 102: return result_codes::LEASE_UNKNOWN_RESOURCE;
        case 103: return result_codes::LEASE_PRIORITY_NOT_ALLOWED;
        case 104: return result_codes::LEASE_RESOURCE_BUSY;
        case 105: return result_codes::LEASE_NOT_FOUND;
        case 106: return result_codes::LEASE_EXPIRED;
        case 107: return result_codes::LEASE_OPERATION_ID_CONFLICT;
        case 108: return result_codes::LEASE_AUTHORITY_ERROR;
        case 109: return result_codes::LEASE_SOURCE_ALREADY_ACTIVE;
        case 110: return result_codes::LEASE_AUTHORITY_BUSY;
        default: return result_codes::INVALID_AURORA_RESPONSE;
    }
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
        MotorConfigurationStateHandler motor_configuration_state;
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
        motor_configuration_state.reset();
        estimator_data.reset();
        error_codes.reset();
        last_motion_command.reset();
        last_control_group_command.reset();
        last_motor_configuration_command.reset();
        last_sub_controller_command.reset();
        aurora_match_observed = false;
        aurora_last_matched = false;
        aurora_match_generation = 0;
        aurora_sample_generation = 0;
    }

    void clearLeaseLocked() {
        lease_state = LeaseState::Unregistered;
        lease_loss_reason = LeaseLossReason::None;
        lease_id = 0;
        lease_resources.clear();
        lease_priority = LeasePriority::Normal;
        lease_duration = std::chrono::milliseconds{0};
        lease_deadline = {};
        next_renewal = {};
        command_sequences.fill(0);
    }

    LeaseResult leaseSnapshotLocked(OperationResult operation) const {
        LeaseResult result;
        result.operation = std::move(operation);
        result.state = lease_state;
        const auto& current_options = session_options ? *session_options : options;
        result.source_id = current_options.source_id;
        result.source_name = detail::effectiveSourceName(current_options);
        result.lease_id = lease_id;
        result.resources = lease_resources;
        result.priority = lease_priority;
        result.granted_duration = lease_duration;
        result.loss_reason = lease_loss_reason;
        if (lease_state == LeaseState::Active) {
            const auto now = std::chrono::steady_clock::now();
            if (now < lease_deadline) {
                result.remaining_duration =
                    std::chrono::duration_cast<std::chrono::milliseconds>(
                        lease_deadline - now);
            }
        }
        return result;
    }

    LeaseResult observedLeaseLocked() const {
        uint32_t code = result_codes::SUCCESS;
        if (lease_state == LeaseState::Registering ||
            lease_state == LeaseState::Replacing ||
            lease_state == LeaseState::Releasing) {
            code = result_codes::LEASE_OPERATION_IN_PROGRESS;
        } else if (lease_state == LeaseState::Lost) {
            code = result_codes::LEASE_LOST;
        }
        return leaseSnapshotLocked(makeResult(code));
    }

    void markLeaseLostLocked(LeaseLossReason reason) {
        lease_state = LeaseState::Lost;
        lease_loss_reason = reason;
        next_renewal = {};
        lease_wake.notify_all();
        state_changed.notify_all();
    }

    void initializeLeaseSession() {
        std::random_device random;
        const uint64_t value =
            (static_cast<uint64_t>(random()) << 32U) ^
            static_cast<uint64_t>(random());
        if (value == 0) {
            throw AuroraSdkError("failed to create lease client instance identity");
        }
        std::lock_guard<std::mutex> lock(mutex);
        client_instance_id = value;
        next_operation_id = value ^ 0xa5a5a5a5a5a5a5a5ULL;
        if (next_operation_id == 0) {
            next_operation_id = 1;
        }
        clearLeaseLocked();
    }

    uint64_t allocateOperationIdLocked() {
        if (next_operation_id == 0 ||
            next_operation_id == std::numeric_limits<uint64_t>::max()) {
            markLeaseLostLocked(LeaseLossReason::InternalError);
            throw AuroraSdkError("lease Operation ID allocator exhausted");
        }
        return next_operation_id++;
    }

    std::optional<detail::LeaseToken> commandTokenLocked(
        const std::vector<AuroraResource>& required,
        std::size_t stream) {
        const auto now = std::chrono::steady_clock::now();
        if (lease_state == LeaseState::Active && now >= lease_deadline) {
            markLeaseLostLocked(LeaseLossReason::DeadlineExpired);
        }
        if (lease_state != LeaseState::Active) {
            return std::nullopt;
        }
        for (const auto resource : required) {
            if (std::find(lease_resources.begin(), lease_resources.end(), resource) ==
                lease_resources.end()) {
                return std::nullopt;
            }
        }
        return detail::LeaseToken{
            session_options->source_id, lease_id, command_sequences.at(stream)};
    }

    void emitLeaseChanged(const LeaseResult& result) {
        LeaseChangedHandler callback;
        {
            std::lock_guard<std::mutex> lock(mutex);
            callback = lease_changed_handler;
        }
        if (!callback) {
            return;
        }
        try {
            detail::UserCallbackScope callback_scope;
            callback(result);
        } catch (const std::exception& error) {
            std::cerr << "[fourier_aurora_sdk] lease callback failed: "
                      << error.what() << std::endl;
        } catch (...) {
            std::cerr << "[fourier_aurora_sdk] lease callback failed" << std::endl;
        }
    }

    LeaseResult manageLease(
        uint32_t action,
        const std::vector<AuroraResource>& resources,
        const LeaseOption& option,
        bool automatic_renew = false) {
        OperationResult admission_failure;
        auto admission = beginOperation(Endpoint::ManageLeaseService, admission_failure);
        if (!admission) {
            std::lock_guard<std::mutex> lock(mutex);
            return leaseSnapshotLocked(admission_failure);
        }
        if (!endpointMatched(admission, Endpoint::ManageLeaseService)) {
            std::lock_guard<std::mutex> lock(mutex);
            return leaseSnapshotLocked(makeResult(result_codes::ENDPOINT_UNMATCHED));
        }

        detail::LeaseManagementRequest request;
        LeaseState previous_state{LeaseState::Unregistered};
        std::chrono::steady_clock::time_point previous_deadline{};
        LeaseResult transition_snapshot;
        {
            std::lock_guard<std::mutex> lock(mutex);
            const auto now = std::chrono::steady_clock::now();
            if (lease_state == LeaseState::Active && now >= lease_deadline) {
                markLeaseLostLocked(LeaseLossReason::DeadlineExpired);
            }
            previous_state = lease_state;
            previous_deadline = lease_deadline;

            if (action == 0) {
                if (lease_state == LeaseState::Active) {
                    return leaseSnapshotLocked(
                        makeResult(result_codes::LEASE_SOURCE_ALREADY_ACTIVE));
                }
                if (lease_state == LeaseState::Registering ||
                    lease_state == LeaseState::Replacing ||
                    lease_state == LeaseState::Releasing) {
                    return leaseSnapshotLocked(
                        makeResult(result_codes::LEASE_OPERATION_IN_PROGRESS));
                }
                lease_state = LeaseState::Registering;
                lease_loss_reason = LeaseLossReason::None;
            } else if (action == 1) {
                if (lease_state != LeaseState::Active) {
                    return leaseSnapshotLocked(
                        makeResult(result_codes::LEASE_NOT_ACTIVE));
                }
            } else if (action == 3) {
                if (lease_state != LeaseState::Active) {
                    return leaseSnapshotLocked(
                        makeResult(result_codes::LEASE_NOT_ACTIVE));
                }
                lease_state = LeaseState::Replacing;
            } else if (action == 2) {
                if (lease_state == LeaseState::Unregistered) {
                    return leaseSnapshotLocked(makeResult(result_codes::SUCCESS));
                }
                if (lease_state == LeaseState::Registering ||
                    lease_state == LeaseState::Replacing ||
                    lease_state == LeaseState::Releasing) {
                    return leaseSnapshotLocked(
                        makeResult(result_codes::LEASE_OPERATION_IN_PROGRESS));
                }
                lease_state = LeaseState::Releasing;
            }

            request.source_id = session_options->source_id;
            request.source_name = detail::effectiveSourceName(*session_options);
            request.client_instance_id = client_instance_id;
            request.action = action;
            request.operation_id = allocateOperationIdLocked();
            request.lease_id = action == 0 ? 0 : lease_id;
            const auto& sent_resources = action == 1 ? lease_resources : resources;
            if (action != 2) {
                request.resources = resourceNames(sent_resources);
                request.priority = static_cast<uint32_t>(
                    action == 1 ? lease_priority :
                    (option.priority.has_value() ? *option.priority : lease_priority));
                request.renew_duration =
                    std::chrono::duration<double>(option.duration).count();
            }
            transition_snapshot = observedLeaseLocked();
        }
        if (!automatic_renew) {
            emitLeaseChanged(transition_snapshot);
        }

        const auto bounded_timeout = static_cast<uint32_t>(std::min<int64_t>(
            option.timeout.count(), std::numeric_limits<uint32_t>::max()));
        const auto wire = admission.transport()->manageLease(request, bounded_timeout);
        LeaseResult result;
        {
            std::lock_guard<std::mutex> lock(mutex);
            if (!wire.operation.success()) {
                if (action == 1 && std::chrono::steady_clock::now() < lease_deadline) {
                    result = leaseSnapshotLocked(wire.operation);
                } else if (action == 3 &&
                           wire.operation.code == result_codes::ENDPOINT_UNMATCHED &&
                           std::chrono::steady_clock::now() < previous_deadline) {
                    lease_state = LeaseState::Active;
                    result = leaseSnapshotLocked(wire.operation);
                } else {
                    markLeaseLostLocked(
                        wire.operation.code == result_codes::SERVICE_TIMEOUT
                            ? LeaseLossReason::OperationTimeout
                            : LeaseLossReason::AuthorityRejected);
                    result = leaseSnapshotLocked(wire.operation);
                }
            } else {
                const uint32_t code = mapLeaseWireResult(wire.wire_result);
                OperationResult operation{code, wire.message.empty()
                    ? std::string(defaultResultMessage(code)) : wire.message};
                const bool accepted = code == result_codes::SUCCESS;
                if (accepted) {
                    if (action == 2) {
                        if (wire.lease_id != request.lease_id ||
                            wire.renew_duration != 0.0) {
                            markLeaseLostLocked(LeaseLossReason::InvalidAuthorityResponse);
                            result = leaseSnapshotLocked(
                                makeResult(result_codes::INVALID_AURORA_RESPONSE));
                        } else {
                            clearLeaseLocked();
                            result = leaseSnapshotLocked(operation);
                        }
                    } else {
                        const double millis = wire.renew_duration * 1000.0;
                        const auto rounded = std::llround(millis);
                        const bool valid_duration = std::isfinite(millis) && rounded >= 100 &&
                            std::abs(millis - static_cast<double>(rounded)) <= 1e-9 &&
                            rounded <= option.duration.count();
                        const bool valid_id = wire.lease_id != 0 &&
                            (action != 1 || wire.lease_id == request.lease_id);
                        if (!valid_duration || !valid_id) {
                            markLeaseLostLocked(LeaseLossReason::InvalidAuthorityResponse);
                            result = leaseSnapshotLocked(
                                makeResult(result_codes::INVALID_AURORA_RESPONSE));
                        } else {
                            lease_state = LeaseState::Active;
                            lease_loss_reason = LeaseLossReason::None;
                            lease_id = wire.lease_id;
                            if (action != 1) {
                                lease_resources = resources;
                                lease_priority = static_cast<LeasePriority>(request.priority);
                                command_sequences.fill(0);
                            }
                            lease_duration = std::chrono::milliseconds{rounded};
                            lease_deadline = std::chrono::steady_clock::now() + lease_duration;
                            next_renewal = std::chrono::steady_clock::now() + lease_duration / 4;
                            lease_wake.notify_all();
                            result = leaseSnapshotLocked(operation);
                        }
                    }
                } else if (action == 2 &&
                           (code == result_codes::LEASE_NOT_FOUND ||
                            code == result_codes::LEASE_EXPIRED)) {
                    clearLeaseLocked();
                    result = leaseSnapshotLocked(makeResult(result_codes::SUCCESS));
                } else if (action == 3 && std::chrono::steady_clock::now() < previous_deadline) {
                    lease_state = LeaseState::Active;
                    result = leaseSnapshotLocked(operation);
                } else if (action == 0) {
                    lease_state = LeaseState::Unregistered;
                    result = leaseSnapshotLocked(operation);
                } else {
                    markLeaseLostLocked(
                        code == result_codes::INVALID_AURORA_RESPONSE
                            ? LeaseLossReason::InvalidAuthorityResponse
                            : LeaseLossReason::AuthorityRejected);
                    result = leaseSnapshotLocked(operation);
                }
            }
        }
        emitLeaseChanged(result);
        return result;
    }

    void renewalLoop() {
        std::unique_lock<std::mutex> lock(mutex);
        while (!renewal_stop) {
            if (lease_state != LeaseState::Active) {
                lease_wake.wait(lock, [this] {
                    return renewal_stop || lease_state == LeaseState::Active;
                });
                continue;
            }
            const auto target = next_renewal;
            if (lease_wake.wait_until(lock, target, [this, target] {
                    return renewal_stop || lease_state != LeaseState::Active ||
                           next_renewal != target;
                })) {
                continue;
            }
            if (std::chrono::steady_clock::now() >= lease_deadline) {
                markLeaseLostLocked(LeaseLossReason::DeadlineExpired);
                const auto snapshot = observedLeaseLocked();
                lock.unlock();
                emitLeaseChanged(snapshot);
                lock.lock();
                continue;
            }
            LeaseOption option;
            option.duration = lease_duration;
            option.priority = lease_priority;
            option.timeout = std::max(
                std::chrono::milliseconds{1}, lease_duration / 8);
            const auto current_resources = lease_resources;
            next_renewal = std::chrono::steady_clock::now() +
                           std::min(std::chrono::milliseconds{100}, lease_duration / 8);
            lock.unlock();
            (void)manageLease(1, current_resources, option, true);
            lock.lock();
        }
    }

    void startRenewalThread() {
        std::lock_guard<std::mutex> lock(mutex);
        if (renewal_thread.joinable()) {
            return;
        }
        renewal_stop = false;
        renewal_thread = std::thread([this] { renewalLoop(); });
    }

    void stopRenewalThread() noexcept {
        {
            std::lock_guard<std::mutex> lock(mutex);
            renewal_stop = true;
            lease_wake.notify_all();
        }
        if (renewal_thread.joinable()) {
            renewal_thread.join();
        }
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
        handlers.motor_configuration_state =
            [this, generation, callback = callbacks.motor_configuration_state](
                MotorConfigurationState state) mutable {
                handleSample(
                    generation,
                    &Impl::motor_configuration_state,
                    callback,
                    std::move(state),
                    "MotorConfigurationState");
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
    std::optional<MotorConfigurationState> motor_configuration_state;
    std::optional<EstimatorData> estimator_data;
    std::optional<ErrorCodes> error_codes;
    std::optional<MotionCommand> last_motion_command;
    std::optional<ControlGroupCommand> last_control_group_command;
    std::optional<MotorConfigurationCommand> last_motor_configuration_command;
    std::optional<SubControllerCommand> last_sub_controller_command;

    LeaseState lease_state{LeaseState::Unregistered};
    LeaseLossReason lease_loss_reason{LeaseLossReason::None};
    uint64_t client_instance_id{0};
    uint64_t next_operation_id{1};
    uint64_t lease_id{0};
    std::vector<AuroraResource> lease_resources;
    LeasePriority lease_priority{LeasePriority::Normal};
    std::chrono::milliseconds lease_duration{0};
    std::chrono::steady_clock::time_point lease_deadline{};
    std::chrono::steady_clock::time_point next_renewal{};
    std::array<uint64_t, 3> command_sequences{};
    LeaseChangedHandler lease_changed_handler;
    std::condition_variable lease_wake;
    bool renewal_stop{true};
    std::thread renewal_thread;
    std::array<std::mutex, 3> command_mutexes;
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
        impl_->stopRenewalThread();
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
    impl_->options.domain_id = domain_id;
}

// Reads the configured DDS domain independently of transport state.
uint32_t AuroraClient::domainId() const {
    std::lock_guard<std::mutex> lock(impl_->mutex);
    return impl_->options.domain_id;
}

// Reports only the local lifecycle state, not endpoint or robot readiness.
ClientState AuroraClient::state() const {
    std::lock_guard<std::mutex> lock(impl_->mutex);
    return impl_->client_state;
}

// Creates a transport session from a frozen copy of the current configuration.
void AuroraClient::start() {
    {
        std::lock_guard<std::mutex> lock(impl_->mutex);
        if (impl_->client_state == ClientState::Running) {
            return;
        }
    }
    impl_->start();
    try {
        impl_->initializeLeaseSession();
        impl_->startRenewalThread();
    } catch (...) {
        impl_->stopRenewalThread();
        impl_->stop();
        throw;
    }
}

// Synchronously drains callbacks and operations, then destroys transport resources.
void AuroraClient::stop() {
    impl_->stopRenewalThread();
    bool should_release = false;
    {
        std::lock_guard<std::mutex> lock(impl_->mutex);
        should_release = impl_->client_state == ClientState::Running &&
                         impl_->lease_state != LeaseState::Unregistered;
    }
    if (should_release) {
        try {
            (void)releaseLease();
        } catch (const std::exception& error) {
            std::cerr << "[fourier_aurora_sdk] lease release during stop failed: "
                      << error.what() << std::endl;
        }
    }
    impl_->stop();
    std::lock_guard<std::mutex> lock(impl_->mutex);
    impl_->clearLeaseLocked();
}

// Takes one enabled/matched snapshot across the configured endpoint set.
EndpointMatchStatus AuroraClient::endpointMatchStatus() const {
    return impl_->endpointMatchStatus();
}

// Provides the common single-endpoint readiness check for the FSM service.
bool AuroraClient::isFsmServiceMatched() const {
    return endpointMatchStatus().fsm_service.matched;
}

// Polls local match state, then allows volatile writers to deliver initial data.
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
    auto next_progress_log = start;

    for (;;) {
        auto status = endpointMatchStatus();
        {
            std::lock_guard<std::mutex> lock(impl_->mutex);
            if (impl_->client_state != ClientState::Running) {
                return false;
            }
        }
        if (allEnabledEndpointsMatched(status)) {
            std::unique_lock<std::mutex> lock(impl_->mutex);
            const bool stopped = impl_->state_changed.wait_for(
                lock,
                kEndpointMatchStabilizationDelay,
                [this] { return impl_->client_state != ClientState::Running; });
            if (stopped) {
                return false;
            }
            lock.unlock();
            status = endpointMatchStatus();
            if (allEnabledEndpointsMatched(status)) {
                std::cerr
                    << "[fourier_aurora_sdk] INFO: all enabled endpoints "
                       "matched; "
                    << formatEndpointMatchSummary(status) << std::endl;
                return true;
            }
        }

        const auto now = std::chrono::steady_clock::now();
        if (now >= deadline) {
            std::cerr
                << "[fourier_aurora_sdk] ERROR: endpoint match timed out "
                   "after "
                << timeout.count() << " ms; "
                << formatEndpointMatchSummary(status) << std::endl;
            return false;
        }
        if (now >= next_progress_log) {
            std::cerr
                << "[fourier_aurora_sdk] INFO: waiting for endpoint "
                   "matches; "
                << formatEndpointMatchSummary(status) << std::endl;
            next_progress_log = now + kEndpointMatchProgressPeriod;
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

OperationResult AuroraClient::getMotorConfigurationState(
    MotorConfigurationState& output) const {
    return impl_->readCached(
        Endpoint::MotorConfigurationStateSubscriber,
        &Impl::motor_configuration_state,
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

OperationResult AuroraClient::getLastMotorConfigurationCommand(
    MotorConfigurationCommand& output) const {
    return impl_->readCached(
        Endpoint::MotorConfigurationCommandPublisher,
        &Impl::last_motor_configuration_command,
        output);
}

// Returns the last sub-controller command successfully transmitted by this client.
OperationResult AuroraClient::getLastSubControllerCommand(
    SubControllerCommand& output) const {
    return impl_->readCached(
        Endpoint::SubControllerService,
        &Impl::last_sub_controller_command,
        output);
}

// Stamps, publishes, and caches one complete motion command.
OperationResult AuroraClient::publishMotionCommand(
    const MotionCommand& command) {
    std::lock_guard<std::mutex> stream_lock(impl_->command_mutexes[0]);
    OperationResult failure;
    auto admission = impl_->beginOperation(
        Endpoint::MotionCommandPublisher, failure);
    if (!admission) {
        return failure;
    }

    detail::LeaseToken token;
    {
        std::lock_guard<std::mutex> lock(impl_->mutex);
        const auto current = impl_->commandTokenLocked(
            {AuroraResource::MotionCommand}, 0);
        if (!current.has_value()) {
            const uint32_t code = impl_->lease_state == LeaseState::Active
                ? result_codes::LEASE_RESOURCE_NOT_COVERED
                : (impl_->lease_state == LeaseState::Lost
                       ? result_codes::LEASE_LOST
                       : (impl_->lease_state == LeaseState::Registering ||
                          impl_->lease_state == LeaseState::Replacing ||
                          impl_->lease_state == LeaseState::Releasing
                              ? result_codes::LEASE_OPERATION_IN_PROGRESS
                              : result_codes::LEASE_NOT_ACTIVE));
            return makeResult(code);
        }
        token = *current;
    }
    MotionCommand transmitted = command;
    transmitted.header.send_time = std::chrono::system_clock::now();
    transmitted.header.frame_id.clear();
    transmitted.header.received_time = {};
    const auto result =
        admission.transport()->publishMotionCommand(transmitted, token);
    if (result.success()) {
        std::lock_guard<std::mutex> lock(impl_->mutex);
        impl_->last_motion_command = transmitted;
        if (impl_->command_sequences[0] != std::numeric_limits<uint64_t>::max()) {
            ++impl_->command_sequences[0];
        }
        impl_->state_changed.notify_all();
    }
    return result;
}

// Stamps, publishes, and caches one complete control-group command.
OperationResult AuroraClient::publishControlGroupCommand(
    const ControlGroupCommand& command) {
    const std::vector<AuroraResource> required = [&] {
        std::vector<AuroraResource> values;
        values.reserve(command.joint_cmd.size());
        for (const auto& group : command.joint_cmd) {
            const auto resource = groupResource(group.group_name);
            if (!resource.has_value()) {
                throw InvalidArgumentError(
                    "unsupported control group: " + group.group_name);
            }
            values.push_back(*resource);
        }
        return canonicalResources(values);
    }();
    std::lock_guard<std::mutex> stream_lock(impl_->command_mutexes[1]);
    OperationResult failure;
    auto admission = impl_->beginOperation(
        Endpoint::ControlGroupCommandPublisher, failure);
    if (!admission) {
        return failure;
    }

    detail::LeaseToken token;
    {
        std::lock_guard<std::mutex> lock(impl_->mutex);
        const auto current = impl_->commandTokenLocked(required, 1);
        if (!current.has_value()) {
            return makeResult(impl_->lease_state == LeaseState::Active
                ? result_codes::LEASE_RESOURCE_NOT_COVERED
                : (impl_->lease_state == LeaseState::Lost
                       ? result_codes::LEASE_LOST
                       : result_codes::LEASE_NOT_ACTIVE));
        }
        token = *current;
    }
    ControlGroupCommand transmitted = command;
    transmitted.header.send_time = std::chrono::system_clock::now();
    transmitted.header.frame_id.clear();
    transmitted.header.received_time = {};
    const auto result =
        admission.transport()->publishControlGroupCommand(transmitted, token);
    if (result.success()) {
        std::lock_guard<std::mutex> lock(impl_->mutex);
        impl_->last_control_group_command = transmitted;
        if (impl_->command_sequences[1] != std::numeric_limits<uint64_t>::max()) {
            ++impl_->command_sequences[1];
        }
        impl_->state_changed.notify_all();
    }
    return result;
}

// Stamps, publishes, and caches one complete sub-controller command.
OperationResult AuroraClient::publishMotorConfigurationCommand(
    const MotorConfigurationCommand& command) {
    const std::vector<AuroraResource> required = [&] {
        std::vector<AuroraResource> values;
        values.reserve(command.groups.size());
        for (const auto& group : command.groups) {
            const auto resource = groupResource(group.group_name);
            if (!resource.has_value() ||
                *resource == AuroraResource::LeftEndEffector ||
                *resource == AuroraResource::RightEndEffector) {
                throw InvalidArgumentError(
                    "unsupported motor configuration group: " + group.group_name);
            }
            values.push_back(*resource);
        }
        return canonicalResources(values);
    }();
    std::lock_guard<std::mutex> stream_lock(impl_->command_mutexes[2]);
    OperationResult failure;
    auto admission = impl_->beginOperation(
        Endpoint::MotorConfigurationCommandPublisher, failure);
    if (!admission) {
        return failure;
    }
    detail::LeaseToken token;
    {
        std::lock_guard<std::mutex> lock(impl_->mutex);
        const auto current = impl_->commandTokenLocked(required, 2);
        if (!current.has_value()) {
            return makeResult(impl_->lease_state == LeaseState::Active
                ? result_codes::LEASE_RESOURCE_NOT_COVERED
                : (impl_->lease_state == LeaseState::Lost
                       ? result_codes::LEASE_LOST
                       : result_codes::LEASE_NOT_ACTIVE));
        }
        token = *current;
    }
    MotorConfigurationCommand transmitted = command;
    transmitted.header.send_time = std::chrono::system_clock::now();
    transmitted.header.frame_id.clear();
    transmitted.header.received_time = {};
    const auto result = admission.transport()->publishMotorConfigurationCommand(
        transmitted, token);
    if (result.success()) {
        std::lock_guard<std::mutex> lock(impl_->mutex);
        impl_->last_motor_configuration_command = transmitted;
        if (impl_->command_sequences[2] != std::numeric_limits<uint64_t>::max()) {
            ++impl_->command_sequences[2];
        }
        impl_->state_changed.notify_all();
    }
    return result;
}

OperationResult AuroraClient::setSubController(
    const SubControllerCommand& command) {
    OperationResult failure;
    auto admission = impl_->beginOperation(
        Endpoint::SubControllerService, failure);
    if (!admission) {
        return failure;
    }

    SubControllerCommand transmitted = command;
    transmitted.header.send_time = std::chrono::system_clock::now();
    transmitted.header.frame_id.clear();
    transmitted.header.received_time = {};
    const auto result =
        admission.transport()->setSubController(transmitted);
    if (result.success()) {
        std::lock_guard<std::mutex> lock(impl_->mutex);
        impl_->last_sub_controller_command = transmitted;
        impl_->state_changed.notify_all();
    }
    return result;
}

LeaseResult AuroraClient::registerLease(
    const std::vector<AuroraResource>& resources,
    const LeaseOption& option) {
    validateLeaseOption(option);
    const auto canonical = canonicalResources(resources);
    LeaseOption effective = option;
    if (!effective.priority.has_value()) {
        effective.priority = LeasePriority::Normal;
    }
    {
        std::lock_guard<std::mutex> lock(impl_->mutex);
        if (*effective.priority == LeasePriority::Emergency &&
            !(impl_->session_options ? impl_->session_options->allow_emergency_lease_priority
                                    : impl_->options.allow_emergency_lease_priority)) {
            throw InvalidArgumentError(
                "Emergency lease priority requires connection opt-in");
        }
    }
    return impl_->manageLease(0, canonical, effective);
}

LeaseResult AuroraClient::replaceLease(
    const std::vector<AuroraResource>& resources,
    const LeaseOption& option) {
    validateLeaseOption(option);
    const auto canonical = canonicalResources(resources);
    LeaseOption effective = option;
    {
        std::lock_guard<std::mutex> lock(impl_->mutex);
        if (!effective.priority.has_value()) {
            effective.priority = impl_->lease_priority;
        }
        if (*effective.priority == LeasePriority::Emergency &&
            !(impl_->session_options ? impl_->session_options->allow_emergency_lease_priority
                                    : impl_->options.allow_emergency_lease_priority)) {
            throw InvalidArgumentError(
                "Emergency lease priority requires connection opt-in");
        }
        if (impl_->lease_state == LeaseState::Active &&
            canonical == impl_->lease_resources &&
            *effective.priority == impl_->lease_priority &&
            effective.duration == impl_->lease_duration) {
            throw InvalidArgumentError("replacement must change the lease contract");
        }
    }
    return impl_->manageLease(3, canonical, effective);
}

LeaseResult AuroraClient::releaseLease() {
    LeaseOption option;
    option.timeout = std::chrono::milliseconds{500};
    std::vector<AuroraResource> current;
    {
        std::lock_guard<std::mutex> lock(impl_->mutex);
        // Release is intentionally idempotent.  This also keeps cleanup simple
        // for callers whose lease has already expired or been preempted.
        if (impl_->lease_state == LeaseState::Unregistered) {
            return impl_->observedLeaseLocked();
        }
        current = impl_->lease_resources;
    }
    return impl_->manageLease(2, current, option);
}

LeaseResult AuroraClient::getLease() const {
    std::lock_guard<std::mutex> lock(impl_->mutex);
    if (impl_->lease_state == LeaseState::Active &&
        std::chrono::steady_clock::now() >= impl_->lease_deadline) {
        impl_->markLeaseLostLocked(LeaseLossReason::DeadlineExpired);
    }
    return impl_->observedLeaseLocked();
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
    return setSubController(command);
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

void AuroraClient::onMotorConfigurationState(
    MotorConfigurationStateHandler callback) {
    std::lock_guard<std::mutex> lock(impl_->mutex);
    if (impl_->client_state != ClientState::Stopped) {
        throw InvalidStateError(
            "callback overrides can change only while the client is stopped");
    }
    impl_->callback_overrides.motor_configuration_state = std::move(callback);
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

void AuroraClient::onLeaseChanged(LeaseChangedHandler callback) {
    std::lock_guard<std::mutex> lock(impl_->mutex);
    if (impl_->client_state != ClientState::Stopped) {
        throw InvalidStateError(
            "lease callback can change only while the client is stopped");
    }
    impl_->lease_changed_handler = std::move(callback);
}

}  // namespace fourier::aurora_sdk
