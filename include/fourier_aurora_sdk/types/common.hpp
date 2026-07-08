#pragma once

#include <chrono>
#include <cstdint>
#include <string>

namespace fourier::aurora_sdk {

struct MessageHeader {
    std::chrono::system_clock::time_point send_time{};
    std::string frame_id;
    std::chrono::system_clock::time_point received_time{};
};

struct ServiceCallResult {
    std::chrono::system_clock::time_point request_send_time{};
    std::chrono::system_clock::time_point response_receive_time{};
    int32_t result_code = -1;
    std::string message;
    bool timed_out = false;
    bool transport_error = false;

    [[nodiscard]] bool success() const {
        return !timed_out && !transport_error && result_code == 0;
    }
};

}  // namespace fourier::aurora_sdk
