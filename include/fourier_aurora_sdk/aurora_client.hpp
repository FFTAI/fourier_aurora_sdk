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

/**
 * Process-wide, restartable high-level Aurora client.
 *
 * Atomic APIs operate on one complete typed endpoint value. Abstract APIs
 * project or construct simpler values through an atomic API. "Atomic" describes
 * the API boundary, not a concurrency transaction guarantee.
 */
class AuroraClient {
public:
  // Client functions operate on the process-wide instance, local
  // configuration, lifecycle, and endpoint readiness.
  static AuroraClient &getInstance();

  AuroraClient(const AuroraClient &) = delete;
  AuroraClient &operator=(const AuroraClient &) = delete;
  AuroraClient(AuroraClient &&) = delete;
  AuroraClient &operator=(AuroraClient &&) = delete;

  [[nodiscard]] ConnectionOptions config() const;
  void configure(const ConnectionOptions &options);
  void setDomainId(uint32_t domain_id);
  [[nodiscard]] uint32_t domainId() const;

  [[nodiscard]] ClientState state() const;
  void start();
  void stop();

  [[nodiscard]] EndpointMatchStatus endpointMatchStatus() const;
  [[nodiscard]] bool isFsmServiceMatched() const;
  [[nodiscard]] bool waitForEndpoints(std::chrono::milliseconds timeout);

  // Atomic getters return one complete cached value from a typed endpoint.
  [[nodiscard]] OperationResult getAuroraState(AuroraState &output) const;
  [[nodiscard]] OperationResult getMotionState(MotionState &output) const;
  [[nodiscard]] OperationResult
  getControlGroupState(ControlGroupState &output) const;
  [[nodiscard]] OperationResult
  getMotorConfigurationState(MotorConfigurationState &output) const;
  [[nodiscard]] OperationResult getEstimatorData(EstimatorData &output) const;
  [[nodiscard]] OperationResult getErrorCodes(ErrorCodes &output) const;

  // Abstract getters and waits expose a focused view derived from one atomic
  // snapshot, preserving single-snapshot consistency.
  [[nodiscard]] OperationResult getBaseData(BaseData &output) const;
  [[nodiscard]] OperationResult getContactData(ContactData &output) const;
  [[nodiscard]] OperationResult getStableLevel(uint8_t &output) const;
  [[nodiscard]] OperationResult getStandPose(std::vector<double> &output) const;
  [[nodiscard]] OperationResult
  getJointState(const std::string &group_name, JointField field,
                std::vector<double> &output) const;
  [[nodiscard]] OperationResult
  getJointCommand(const std::string &group_name, JointField field,
                  std::vector<double> &output) const;
  [[nodiscard]] OperationResult
  getCartesianState(const std::string &link_name,
                    CartesianStateEntry &output) const;
  [[nodiscard]] OperationResult
  getCartesianCommand(const std::string &link_name,
                      CartesianCommand &output) const;
  // Compatibility overload. Omitted sparse fields are returned as zero arrays.
  [[nodiscard]] OperationResult
  getCartesianCommand(const std::string &link_name,
                      CartesianStateEntry &output) const;
  [[nodiscard]] bool waitForFsmState(uint32_t state_id,
                                     std::chrono::milliseconds timeout);

  // Client cache getters return commands last published by this client,
  // rather than robot-reported state.
  [[nodiscard]] OperationResult
  getLastMotionCommand(MotionCommand &output) const;
  [[nodiscard]] OperationResult
  getLastControlGroupCommand(ControlGroupCommand &output) const;
  [[nodiscard]] OperationResult
  getLastMotorConfigurationCommand(MotorConfigurationCommand &output) const;
  [[nodiscard]] OperationResult
  getLastSubControllerCommand(SubControllerCommand &output) const;

  // Atomic setters and RPCs send one complete typed value without adding
  // higher-level command semantics.
  [[nodiscard]] OperationResult
  publishMotionCommand(const MotionCommand &command);
  [[nodiscard]] OperationResult
  publishControlGroupCommand(const ControlGroupCommand &command);
  [[nodiscard]] OperationResult
  publishMotorConfigurationCommand(const MotorConfigurationCommand &command);
  [[nodiscard]] OperationResult
  setSubController(const SubControllerCommand &command);
  [[nodiscard]] OperationResult requestFsmState(uint32_t state_id);
  [[nodiscard]] OperationResult requestFsmState(uint32_t state_id,
                                                bool force_switch);

  // Explicitly manages the one module-level lease owned by this client.
  [[nodiscard]] LeaseResult
  registerLease(const std::vector<AuroraResource> &resources,
                const LeaseOption &option = {});
  [[nodiscard]] LeaseResult
  replaceLease(const std::vector<AuroraResource> &resources,
               const LeaseOption &option = {});
  [[nodiscard]] LeaseResult releaseLease();
  [[nodiscard]] LeaseResult getLease() const;

  // Abstract setters construct a typed command from simpler user-facing
  // values, then delegate to an atomic setter.
  [[nodiscard]] OperationResult setVelocity(double vx, double vy, double yaw,
                                            double duration,
                                            uint8_t gait_type = 0);
  [[nodiscard]] OperationResult setStandPose(double delta_height,
                                             double delta_pitch,
                                             double delta_yaw,
                                             double duration = 1.0);
  [[nodiscard]] OperationResult resetUnicontroller();

  // Client callback functions configure local post-cache-update hooks while
  // stopped; callbacks run inline on the DDS callback thread.
  void onAuroraState(AuroraStateHandler callback);
  void onMotionState(MotionStateHandler callback);
  void onControlGroupState(ControlGroupStateHandler callback);
  void onMotorConfigurationState(MotorConfigurationStateHandler callback);
  void onEstimatorData(EstimatorDataHandler callback);
  void onErrorCodes(ErrorCodesHandler callback);
  void onLeaseChanged(LeaseChangedHandler callback);

private:
  // Singleton construction and no-throw fallback cleanup are internal.
  AuroraClient();
  ~AuroraClient() noexcept;

  struct Impl;
  std::unique_ptr<Impl> impl_;
};

} // namespace fourier::aurora_sdk
