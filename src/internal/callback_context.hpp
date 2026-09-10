#pragma once

#include <cstddef>

namespace fourier::aurora_sdk::detail {

inline thread_local std::size_t user_callback_depth = 0;

class UserCallbackScope {
public:
    UserCallbackScope() noexcept {
        ++user_callback_depth;
    }

    ~UserCallbackScope() {
        --user_callback_depth;
    }

    UserCallbackScope(const UserCallbackScope&) = delete;
    UserCallbackScope& operator=(const UserCallbackScope&) = delete;
};

[[nodiscard]] inline bool inUserCallback() noexcept {
    return user_callback_depth != 0;
}

}  // namespace fourier::aurora_sdk::detail
