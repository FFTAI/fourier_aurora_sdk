#pragma once

#include <cstdint>
#include <vector>

#include "fourier_aurora_sdk/types/common.hpp"

namespace fourier::aurora_sdk {

struct ErrorCodeEntry {
    uint32_t high32 = 0;
    uint32_t low32 = 0;
};

struct ErrorCodes {
    MessageHeader header;
    uint8_t source = 0;
    std::vector<ErrorCodeEntry> error_codes;
};

}  // namespace fourier::aurora_sdk
