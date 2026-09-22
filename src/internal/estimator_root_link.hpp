#pragma once

#include <string>

namespace fourier::aurora_sdk::detail {

// Older generated DDS types do not expose root_link(). Keep them buildable.
template <typename Message>
auto estimatorRootLink(const Message& message, int)
    -> decltype(std::string(message.root_link())) {
  return message.root_link();
}

template <typename Message>
std::string estimatorRootLink(const Message&, ...) {
  return {};
}

}  // namespace fourier::aurora_sdk::detail
