#pragma once

#include <chrono>
#include <cstdint>
#include <functional>
#include <optional>
#include <string>
#include <vector>

#include "fourier_aurora_sdk/types/common.hpp"

namespace fourier::aurora_sdk {

enum class AuroraResource : uint8_t {
  LeftLeg = 0,
  RightLeg = 1,
  Waist = 2,
  Head = 3,
  LeftManipulator = 4,
  RightManipulator = 5,
  LeftEndEffector = 6,
  RightEndEffector = 7,
  MotionCommand = 8,
  CartesianCommand = 9,
};

enum class LeasePriority : uint8_t { Normal = 0, Emergency = 9 };

enum class LeaseState : uint8_t {
  Unregistered = 0,
  Registering = 1,
  Active = 2,
  Replacing = 3,
  Releasing = 4,
  Lost = 5,
};

enum class LeaseLossReason : uint8_t {
  None = 0,
  DeadlineExpired = 1,
  AuthorityRejected = 2,
  OperationTimeout = 3,
  InvalidAuthorityResponse = 4,
  PublicationDrainTimeout = 5,
  InternalError = 6,
};

struct LeaseOption {
  std::chrono::milliseconds duration{4000};
  std::optional<LeasePriority> priority;
  std::chrono::milliseconds timeout{500};
};

struct LeaseResult {
  OperationResult operation;
  LeaseState state{LeaseState::Unregistered};
  std::string source_name;
  uint64_t lease_id{0};
  std::vector<AuroraResource> resources;
  LeasePriority priority{LeasePriority::Normal};
  std::chrono::milliseconds granted_duration{0};
  std::chrono::milliseconds remaining_duration{0};
  LeaseLossReason loss_reason{LeaseLossReason::None};

  [[nodiscard]] bool success() const noexcept { return operation.success(); }
  [[nodiscard]] bool usable() const noexcept {
    return state == LeaseState::Active && remaining_duration.count() > 0;
  }
};

struct AuthorityResourceStatus {
  AuroraResource resource{AuroraResource::LeftLeg};
  bool acquired{false};
  std::string holder_source_name;
  LeasePriority holder_priority{LeasePriority::Normal};
};

using LeaseChangedHandler = std::function<void(LeaseResult)>;

} // namespace fourier::aurora_sdk
