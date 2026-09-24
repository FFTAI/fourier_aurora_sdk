#pragma once

#include <cstdint>

namespace fourier::aurora_sdk::detail {

template <typename Message>
auto estimatorHangingLevel(const Message& message, int)
    -> decltype(static_cast<uint8_t>(message.hanging_level())) {
  return static_cast<uint8_t>(message.hanging_level());
}

// Keep the SDK buildable with older generated DDS message types.
template <typename Message>
uint8_t estimatorHangingLevel(const Message&, ...) {
  return 0;
}

}  // namespace fourier::aurora_sdk::detail
