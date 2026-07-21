#pragma once

#include <functional>

#include "fourier_aurora_sdk/types.hpp"

namespace fourier::aurora_sdk {

using AuroraStateHandler = std::function<void(AuroraState)>;
using MotionStateHandler = std::function<void(MotionState)>;
using ControlGroupStateHandler = std::function<void(ControlGroupState)>;
using EstimatorDataHandler = std::function<void(EstimatorData)>;
using ErrorCodesHandler = std::function<void(ErrorCodes)>;

}  // namespace fourier::aurora_sdk
