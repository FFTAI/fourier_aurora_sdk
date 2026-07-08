#include "fourier_aurora_sdk/aurora_client.hpp"

#include <algorithm>
#include <chrono>
#include <iostream>
#include <utility>

#if defined(FOURIER_AURORA_SDK_ENABLE_DDS_TRANSPORT)
#include "fourier_aurora_sdk/transport/fourierdds_transport.hpp"
#endif

namespace fourier::aurora_sdk {

std::mutex AuroraClient::instance_mutex_;
std::unique_ptr<AuroraClient> AuroraClient::instance_;

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

const ControlGroupStateEntry& findJointEntry(
    const std::vector<ControlGroupStateEntry>& entries,
    const std::string& name,
    const char* kind) {
    const auto it = std::find_if(entries.begin(), entries.end(), [&](const ControlGroupStateEntry& entry) {
        return entry.group_name == name;
    });
    if (it == entries.end()) {
        throw InvalidArgumentError(std::string(kind) + " not found: " + name);
    }
    return *it;
}

const CartesianStateEntry& findCartesianEntry(
    const std::vector<CartesianStateEntry>& entries,
    const std::string& name,
    const char* kind) {
    const auto it = std::find_if(entries.begin(), entries.end(), [&](const CartesianStateEntry& entry) {
        return entry.link_name == name;
    });
    if (it == entries.end()) {
        throw InvalidArgumentError(std::string(kind) + " not found: " + name);
    }
    return *it;
}

}  // namespace

AuroraClient& AuroraClient::getInstance(
    std::string user_id,
    uint32_t domain_id,
    ConnectionOptions options) {
    validateUserId(user_id);
    const auto normalized = normalizeConnectionOptions(options);
    std::lock_guard<std::mutex> lock(instance_mutex_);
    if (instance_) {
        if (instance_->user_id_ != user_id || instance_->domain_id_ != domain_id ||
            !(instance_->options_ == normalized)) {
            throw InvalidArgumentError("AuroraClient singleton already exists with different options");
        }
        return *instance_;
    }
#if defined(FOURIER_AURORA_SDK_ENABLE_DDS_TRANSPORT)
    auto transport = std::make_unique<FourierDdsTransport>(domain_id, normalized);
    instance_.reset(new AuroraClient(std::move(user_id), domain_id, normalized, std::move(transport)));
    return *instance_;
#else
    throw ConnectionError(
        "FourierDdsTransport is not linked in this build; use getInstanceForTesting or enable DDS transport");
#endif
}

AuroraClient& AuroraClient::getInstanceForTesting(
    std::string user_id,
    uint32_t domain_id,
    ConnectionOptions options,
    std::unique_ptr<ITransport> transport) {
    validateUserId(user_id);
    if (!transport) {
        throw InvalidArgumentError("transport must not be null");
    }
    const auto normalized = normalizeConnectionOptions(options);
    std::lock_guard<std::mutex> lock(instance_mutex_);
    if (instance_) {
        if (instance_->user_id_ != user_id || instance_->domain_id_ != domain_id ||
            !(instance_->options_ == normalized)) {
            throw InvalidArgumentError("AuroraClient singleton already exists with different options");
        }
        return *instance_;
    }
    instance_.reset(new AuroraClient(std::move(user_id), domain_id, normalized, std::move(transport)));
    return *instance_;
}

AuroraClient::AuroraClient(
    std::string user_id,
    uint32_t domain_id,
    NormalizedConnectionOptions options,
    std::unique_ptr<ITransport> transport)
    : user_id_(std::move(user_id)),
      domain_id_(domain_id),
      options_(std::move(options)),
      transport_(std::move(transport)) {
    installTransportHandlers();
}

AuroraClient::~AuroraClient() {
    shutdown();
}

void AuroraClient::close() {
    std::unique_ptr<AuroraClient> owner;
    {
        std::lock_guard<std::mutex> instance_lock(instance_mutex_);
        if (instance_.get() == this) {
            owner = std::move(instance_);
        }
    }
    shutdown();
}

void AuroraClient::shutdown() {
    std::unique_ptr<ITransport> transport_to_close;
    {
        std::unique_lock<std::mutex> lock(mutex_);
        if (closed_) {
            return;
        }
        shutting_down_ = true;
        aurora_state_callback_ = nullptr;
        motion_state_callback_ = nullptr;
        control_group_state_callback_ = nullptr;
        estimator_data_callback_ = nullptr;
        error_codes_callback_ = nullptr;
        transport_to_close = std::move(transport_);
    }
    if (transport_to_close) {
        transport_to_close->close();
    }
    {
        std::unique_lock<std::mutex> lock(mutex_);
        callbacks_drained_.wait(lock, [&] { return active_callbacks_ == 0; });
        aurora_state_.reset();
        motion_state_.reset();
        control_group_state_.reset();
        estimator_data_.reset();
        error_codes_.reset();
        last_motion_command_.reset();
        last_control_group_command_.reset();
        last_sub_controller_command_.reset();
        closed_ = true;
    }
}

AuroraState AuroraClient::getAuroraState() const {
    std::lock_guard<std::mutex> lock(mutex_);
    return requireCache(aurora_state_, "AuroraState");
}

MotionState AuroraClient::getMotionState() const {
    std::lock_guard<std::mutex> lock(mutex_);
    return requireCache(motion_state_, "MotionState");
}

ControlGroupState AuroraClient::getControlGroupState() const {
    std::lock_guard<std::mutex> lock(mutex_);
    return requireCache(control_group_state_, "ControlGroupState");
}

EstimatorData AuroraClient::getEstimatorData() const {
    std::lock_guard<std::mutex> lock(mutex_);
    return requireCache(estimator_data_, "EstimatorData");
}

ErrorCodes AuroraClient::getErrorCodes() const {
    std::lock_guard<std::mutex> lock(mutex_);
    return requireCache(error_codes_, "ErrorCodes");
}

BaseData AuroraClient::getBaseData() const {
    const auto estimator = getEstimatorData();
    BaseData base;
    base.header = estimator.header;
    base.quat_xyzw = estimator.quat_xyzw;
    base.quat_wxyz = estimator.quat_wxyz;
    base.rpy = estimator.rpy;
    base.omega_W = estimator.omega_W;
    base.acc_W = estimator.acc_W;
    base.omega_B = estimator.omega_B;
    base.acc_B = estimator.acc_B;
    base.vel_W = estimator.vel_W;
    base.pos_W = estimator.pos_W;
    base.vel_B = estimator.vel_B;
    base.stable_level = estimator.stable_level;
    return base;
}

ContactData AuroraClient::getContactData() const {
    const auto estimator = getEstimatorData();
    ContactData contact;
    contact.header = estimator.header;
    contact.contact_force = estimator.contact_force;
    contact.contact_prob = estimator.contact_prob;
    return contact;
}

uint8_t AuroraClient::getStableLevel() const {
    return getMotionState().stable_level;
}

std::vector<double> AuroraClient::getStandPose() const {
    const auto motion = getMotionState();
    return {motion.delta_height_cmd, motion.attitude_cmd[1], motion.attitude_cmd[2],
            static_cast<double>(motion.stable_level)};
}

std::vector<double> AuroraClient::getJointState(const std::string& group_name, JointField field) const {
    const auto state = getControlGroupState();
    return selectJointField(findJointEntry(state.joint_state, group_name, "joint state group"), field);
}

std::vector<double> AuroraClient::getJointCommand(const std::string& group_name, JointField field) const {
    const auto state = getControlGroupState();
    return selectJointField(findJointEntry(state.joint_cmd, group_name, "joint command group"), field);
}

CartesianStateEntry AuroraClient::getCartesianState(const std::string& link_name) const {
    const auto state = getControlGroupState();
    return findCartesianEntry(state.cartesian_state, link_name, "cartesian state link");
}

CartesianStateEntry AuroraClient::getCartesianCommand(const std::string& link_name) const {
    const auto state = getControlGroupState();
    return findCartesianEntry(state.cartesian_cmd, link_name, "cartesian command link");
}

std::optional<MotionCommand> AuroraClient::getLastMotionCommand() const {
    std::lock_guard<std::mutex> lock(mutex_);
    return last_motion_command_;
}

std::optional<ControlGroupCommand> AuroraClient::getLastControlGroupCommand() const {
    std::lock_guard<std::mutex> lock(mutex_);
    return last_control_group_command_;
}

std::optional<SubControllerCommand> AuroraClient::getLastSubControllerCommand() const {
    std::lock_guard<std::mutex> lock(mutex_);
    return last_sub_controller_command_;
}

void AuroraClient::publishMotionCommand(MotionCommand command) {
    command.header.send_time = std::chrono::system_clock::now();
    command.header.frame_id.clear();
    command.header.received_time = {};
    {
        std::lock_guard<std::mutex> lock(mutex_);
        last_motion_command_ = command;
    }
    if (!transport_) {
        throw ConnectionError("transport is closed");
    }
    transport_->publishMotionCommand(command);
}

void AuroraClient::publishControlGroupCommand(ControlGroupCommand command) {
    command.header.send_time = std::chrono::system_clock::now();
    command.header.frame_id.clear();
    command.header.received_time = {};
    {
        std::lock_guard<std::mutex> lock(mutex_);
        last_control_group_command_ = command;
    }
    if (!transport_) {
        throw ConnectionError("transport is closed");
    }
    transport_->publishControlGroupCommand(command);
}

void AuroraClient::publishSubControllerCommand(SubControllerCommand command) {
    command.header.send_time = std::chrono::system_clock::now();
    command.header.frame_id.clear();
    command.header.received_time = {};
    {
        std::lock_guard<std::mutex> lock(mutex_);
        last_sub_controller_command_ = command;
    }
    if (!transport_) {
        throw ConnectionError("transport is closed");
    }
    transport_->publishSubControllerCommand(command);
}

ServiceCallResult AuroraClient::requestFsmState(uint32_t state_id) {
    if (!transport_) {
        throw ConnectionError("transport is closed");
    }
    return transport_->requestFsmState(state_id, options_.service_timeout_ms);
}

void AuroraClient::setVelocity(double vx, double vy, double yaw, double duration, uint8_t gait_type) {
    MotionCommand command;
    command.lin_velocity = {vx, vy, 0.0};
    command.ang_velocity = {0.0, 0.0, yaw};
    command.duration = duration;
    command.gait_type = gait_type;
    publishMotionCommand(command);
}

void AuroraClient::setStandPose(double delta_height, double delta_pitch, double delta_yaw, double duration) {
    MotionCommand command;
    command.delta_height = delta_height;
    command.attitude = {0.0, delta_pitch, delta_yaw};
    command.duration = duration;
    publishMotionCommand(command);
}

void AuroraClient::resetUnicontroller() {
    SubControllerCommand command;
    command.reset = true;
    publishSubControllerCommand(command);
}

void AuroraClient::onAuroraState(AuroraStateHandler callback) {
    std::lock_guard<std::mutex> lock(mutex_);
    aurora_state_callback_ = std::move(callback);
}

void AuroraClient::onMotionState(MotionStateHandler callback) {
    std::lock_guard<std::mutex> lock(mutex_);
    motion_state_callback_ = std::move(callback);
}

void AuroraClient::onControlGroupState(ControlGroupStateHandler callback) {
    std::lock_guard<std::mutex> lock(mutex_);
    control_group_state_callback_ = std::move(callback);
}

void AuroraClient::onEstimatorData(EstimatorDataHandler callback) {
    std::lock_guard<std::mutex> lock(mutex_);
    estimator_data_callback_ = std::move(callback);
}

void AuroraClient::onErrorCodes(ErrorCodesHandler callback) {
    std::lock_guard<std::mutex> lock(mutex_);
    error_codes_callback_ = std::move(callback);
}

bool AuroraClient::isFsmServiceMatched() const {
    return true;
}

void AuroraClient::installTransportHandlers() {
    transport_->setAuroraStateHandler([this](AuroraState state) { handleAuroraState(std::move(state)); });
    transport_->setMotionStateHandler([this](MotionState state) { handleMotionState(std::move(state)); });
    transport_->setControlGroupStateHandler([this](ControlGroupState state) { handleControlGroupState(std::move(state)); });
    transport_->setEstimatorDataHandler([this](EstimatorData data) { handleEstimatorData(std::move(data)); });
    transport_->setErrorCodesHandler([this](ErrorCodes codes) { handleErrorCodes(std::move(codes)); });
}

void AuroraClient::beginCallback() {
    std::lock_guard<std::mutex> lock(mutex_);
    if (shutting_down_) {
        return;
    }
    ++active_callbacks_;
}

void AuroraClient::endCallback() {
    std::lock_guard<std::mutex> lock(mutex_);
    if (active_callbacks_ > 0) {
        --active_callbacks_;
    }
    callbacks_drained_.notify_all();
}

void AuroraClient::handleAuroraState(AuroraState state) {
    beginCallback();
    AuroraStateHandler callback;
    {
        std::lock_guard<std::mutex> lock(mutex_);
        if (!shutting_down_) {
            aurora_state_ = state;
            callback = aurora_state_callback_;
        }
    }
    if (callback) {
        try {
            callback(state);
        } catch (const std::exception& ex) {
            std::cerr << "AuroraState callback failed: " << ex.what() << std::endl;
        }
    }
    endCallback();
}

void AuroraClient::handleMotionState(MotionState state) {
    beginCallback();
    MotionStateHandler callback;
    {
        std::lock_guard<std::mutex> lock(mutex_);
        if (!shutting_down_) {
            motion_state_ = state;
            callback = motion_state_callback_;
        }
    }
    if (callback) {
        try {
            callback(state);
        } catch (const std::exception& ex) {
            std::cerr << "MotionState callback failed: " << ex.what() << std::endl;
        }
    }
    endCallback();
}

void AuroraClient::handleControlGroupState(ControlGroupState state) {
    beginCallback();
    ControlGroupStateHandler callback;
    {
        std::lock_guard<std::mutex> lock(mutex_);
        if (!shutting_down_) {
            control_group_state_ = state;
            callback = control_group_state_callback_;
        }
    }
    if (callback) {
        try {
            callback(state);
        } catch (const std::exception& ex) {
            std::cerr << "ControlGroupState callback failed: " << ex.what() << std::endl;
        }
    }
    endCallback();
}

void AuroraClient::handleEstimatorData(EstimatorData data) {
    beginCallback();
    EstimatorDataHandler callback;
    {
        std::lock_guard<std::mutex> lock(mutex_);
        if (!shutting_down_) {
            estimator_data_ = data;
            callback = estimator_data_callback_;
        }
    }
    if (callback) {
        try {
            callback(data);
        } catch (const std::exception& ex) {
            std::cerr << "EstimatorData callback failed: " << ex.what() << std::endl;
        }
    }
    endCallback();
}

void AuroraClient::handleErrorCodes(ErrorCodes codes) {
    beginCallback();
    ErrorCodesHandler callback;
    {
        std::lock_guard<std::mutex> lock(mutex_);
        if (!shutting_down_) {
            error_codes_ = codes;
            callback = error_codes_callback_;
        }
    }
    if (callback) {
        try {
            callback(codes);
        } catch (const std::exception& ex) {
            std::cerr << "ErrorCodes callback failed: " << ex.what() << std::endl;
        }
    }
    endCallback();
}

}  // namespace fourier::aurora_sdk
