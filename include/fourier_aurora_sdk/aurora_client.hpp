#pragma once

#include <condition_variable>
#include <functional>
#include <memory>
#include <mutex>
#include <optional>
#include <string>

#include "fourier_aurora_sdk/connection_options.hpp"
#include "fourier_aurora_sdk/errors.hpp"
#include "fourier_aurora_sdk/transport/transport.hpp"
#include "fourier_aurora_sdk/types.hpp"

namespace fourier::aurora_sdk {

class AuroraClient {
public:
    static AuroraClient& getInstance(
        std::string user_id,
        uint32_t domain_id = 123,
        ConnectionOptions options = {});
    static AuroraClient& getInstanceForTesting(
        std::string user_id,
        uint32_t domain_id,
        ConnectionOptions options,
        std::unique_ptr<ITransport> transport);

    AuroraClient(const AuroraClient&) = delete;
    AuroraClient& operator=(const AuroraClient&) = delete;
    ~AuroraClient();

    void close();

    [[nodiscard]] const std::string& userId() const { return user_id_; }
    [[nodiscard]] uint32_t domainId() const { return domain_id_; }
    [[nodiscard]] const NormalizedConnectionOptions& options() const { return options_; }

    [[nodiscard]] AuroraState getAuroraState() const;
    [[nodiscard]] MotionState getMotionState() const;
    [[nodiscard]] ControlGroupState getControlGroupState() const;
    [[nodiscard]] EstimatorData getEstimatorData() const;
    [[nodiscard]] ErrorCodes getErrorCodes() const;
    [[nodiscard]] BaseData getBaseData() const;
    [[nodiscard]] ContactData getContactData() const;
    [[nodiscard]] uint8_t getStableLevel() const;

    [[nodiscard]] std::vector<double> getStandPose() const;
    [[nodiscard]] std::vector<double> getJointState(const std::string& group_name, JointField field) const;
    [[nodiscard]] std::vector<double> getJointCommand(const std::string& group_name, JointField field) const;
    [[nodiscard]] CartesianStateEntry getCartesianState(const std::string& link_name) const;
    [[nodiscard]] CartesianStateEntry getCartesianCommand(const std::string& link_name) const;

    [[nodiscard]] std::optional<MotionCommand> getLastMotionCommand() const;
    [[nodiscard]] std::optional<ControlGroupCommand> getLastControlGroupCommand() const;
    [[nodiscard]] std::optional<SubControllerCommand> getLastSubControllerCommand() const;

    void publishMotionCommand(MotionCommand command);
    void publishControlGroupCommand(ControlGroupCommand command);
    void publishSubControllerCommand(SubControllerCommand command);
    ServiceCallResult requestFsmState(uint32_t state_id);

    void setVelocity(double vx, double vy, double yaw, double duration, uint8_t gait_type = 0);
    void setStandPose(double delta_height, double delta_pitch, double delta_yaw, double duration = 1.0);
    void resetUnicontroller();

    void onAuroraState(AuroraStateHandler callback);
    void onMotionState(MotionStateHandler callback);
    void onControlGroupState(ControlGroupStateHandler callback);
    void onEstimatorData(EstimatorDataHandler callback);
    void onErrorCodes(ErrorCodesHandler callback);

    [[nodiscard]] bool isFsmServiceMatched() const;

private:
    AuroraClient(
        std::string user_id,
        uint32_t domain_id,
        NormalizedConnectionOptions options,
        std::unique_ptr<ITransport> transport);

    void shutdown();
    template <typename T>
    T requireCache(const std::optional<T>& cache, const char* name) const {
        if (!cache.has_value()) {
            throw StateNotReadyError(std::string(name) + " has not been received yet");
        }
        return *cache;
    }

    void installTransportHandlers();
    void handleAuroraState(AuroraState state);
    void handleMotionState(MotionState state);
    void handleControlGroupState(ControlGroupState state);
    void handleEstimatorData(EstimatorData data);
    void handleErrorCodes(ErrorCodes codes);
    void beginCallback();
    void endCallback();

    static std::mutex instance_mutex_;
    static std::unique_ptr<AuroraClient> instance_;

    std::string user_id_;
    uint32_t domain_id_;
    NormalizedConnectionOptions options_;
    std::unique_ptr<ITransport> transport_;

    mutable std::mutex mutex_;
    std::condition_variable callbacks_drained_;
    bool closed_ = false;
    bool shutting_down_ = false;
    size_t active_callbacks_ = 0;

    std::optional<AuroraState> aurora_state_;
    std::optional<MotionState> motion_state_;
    std::optional<ControlGroupState> control_group_state_;
    std::optional<EstimatorData> estimator_data_;
    std::optional<ErrorCodes> error_codes_;
    std::optional<MotionCommand> last_motion_command_;
    std::optional<ControlGroupCommand> last_control_group_command_;
    std::optional<SubControllerCommand> last_sub_controller_command_;

    AuroraStateHandler aurora_state_callback_;
    MotionStateHandler motion_state_callback_;
    ControlGroupStateHandler control_group_state_callback_;
    EstimatorDataHandler estimator_data_callback_;
    ErrorCodesHandler error_codes_callback_;
};

}  // namespace fourier::aurora_sdk
