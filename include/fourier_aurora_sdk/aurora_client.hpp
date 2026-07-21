#pragma once

#include <chrono>
#include <cstdint>
#include <memory>
#include <optional>
#include <string>
#include <vector>

#include "fourier_aurora_sdk/callbacks.hpp"
#include "fourier_aurora_sdk/connection_options.hpp"
#include "fourier_aurora_sdk/errors.hpp"
#include "fourier_aurora_sdk/types.hpp"

namespace fourier::aurora_sdk {

class AuroraClient {
public:
    static AuroraClient& getInstance();

    AuroraClient(const AuroraClient&) = delete;
    AuroraClient& operator=(const AuroraClient&) = delete;
    AuroraClient(AuroraClient&&) = delete;
    AuroraClient& operator=(AuroraClient&&) = delete;

    [[nodiscard]] ConnectionOptions config() const;
    void configure(const ConnectionOptions& options);
    void setDomainId(uint32_t domain_id);
    void setUserId(std::string user_id);
    [[nodiscard]] uint32_t domainId() const;
    [[nodiscard]] std::string userId() const;

    [[nodiscard]] ClientState state() const;
    void start();
    void stop();

    [[nodiscard]] EndpointMatchStatus endpointMatchStatus() const;
    [[nodiscard]] bool isFsmServiceMatched() const;
    [[nodiscard]] bool waitForEndpoints(std::chrono::milliseconds timeout);
    [[nodiscard]] bool waitForFsmState(
        uint32_t state_id,
        std::chrono::milliseconds timeout);

    [[nodiscard]] OperationResult getAuroraState(AuroraState& output) const;
    [[nodiscard]] OperationResult getMotionState(MotionState& output) const;
    [[nodiscard]] OperationResult getControlGroupState(
        ControlGroupState& output) const;
    [[nodiscard]] OperationResult getEstimatorData(
        EstimatorData& output) const;
    [[nodiscard]] OperationResult getErrorCodes(ErrorCodes& output) const;
    [[nodiscard]] OperationResult getBaseData(BaseData& output) const;
    [[nodiscard]] OperationResult getContactData(ContactData& output) const;
    [[nodiscard]] OperationResult getStableLevel(uint8_t& output) const;
    [[nodiscard]] OperationResult getStandPose(
        std::vector<double>& output) const;
    [[nodiscard]] OperationResult getJointState(
        const std::string& group_name,
        JointField field,
        std::vector<double>& output) const;
    [[nodiscard]] OperationResult getJointCommand(
        const std::string& group_name,
        JointField field,
        std::vector<double>& output) const;
    [[nodiscard]] OperationResult getCartesianState(
        const std::string& link_name,
        CartesianStateEntry& output) const;
    [[nodiscard]] OperationResult getCartesianCommand(
        const std::string& link_name,
        CartesianStateEntry& output) const;

    [[nodiscard]] OperationResult getLastMotionCommand(
        MotionCommand& output) const;
    [[nodiscard]] OperationResult getLastControlGroupCommand(
        ControlGroupCommand& output) const;
    [[nodiscard]] OperationResult getLastSubControllerCommand(
        SubControllerCommand& output) const;

    [[nodiscard]] OperationResult publishMotionCommand(
        const MotionCommand& command);
    [[nodiscard]] OperationResult publishControlGroupCommand(
        const ControlGroupCommand& command);
    [[nodiscard]] OperationResult publishSubControllerCommand(
        const SubControllerCommand& command);
    [[nodiscard]] OperationResult requestFsmState(uint32_t state_id);

    [[nodiscard]] OperationResult setVelocity(
        double vx,
        double vy,
        double yaw,
        double duration,
        uint8_t gait_type = 0);
    [[nodiscard]] OperationResult setStandPose(
        double delta_height,
        double delta_pitch,
        double delta_yaw,
        double duration = 1.0);
    [[nodiscard]] OperationResult resetUnicontroller();

    void onAuroraState(AuroraStateHandler callback);
    void onMotionState(MotionStateHandler callback);
    void onControlGroupState(ControlGroupStateHandler callback);
    void onEstimatorData(EstimatorDataHandler callback);
    void onErrorCodes(ErrorCodesHandler callback);

private:
    AuroraClient();
    ~AuroraClient() noexcept;

    struct Impl;
    std::unique_ptr<Impl> impl_;
};

}  // namespace fourier::aurora_sdk
