#pragma once

#include <algorithm>
#include <array>
#include <chrono>
#include <cmath>
#include <csignal>
#include <cstdint>
#include <iomanip>
#include <iostream>
#include <limits>
#include <sstream>
#include <string>
#include <thread>
#include <vector>

#include "fourier_aurora_sdk/aurora_client.hpp"

namespace client_example {

inline constexpr int kSuccess = 0;
inline constexpr int kRuntimeFailure = 1;
inline constexpr int kUsageError = 2;
inline constexpr int kInterrupted = 130;

inline volatile std::sig_atomic_t stop_requested = 0;

inline void handleSignal(int) {
    stop_requested = 1;
}

inline void installSignalHandlers() {
    stop_requested = 0;
    struct sigaction action {};
    action.sa_handler = handleSignal;
    sigemptyset(&action.sa_mask);
    action.sa_flags = 0;
    sigaction(SIGINT, &action, nullptr);
    sigaction(SIGTERM, &action, nullptr);
}

[[nodiscard]] inline bool stopRequested() {
    return stop_requested != 0;
}

[[nodiscard]] inline bool parseUint32(
    const std::string& text,
    uint32_t& value) {
    if (text.empty() || text.front() == '-') {
        return false;
    }
    std::size_t consumed = 0;
    unsigned long long parsed = 0;
    try {
        parsed = std::stoull(text, &consumed);
    } catch (const std::exception&) {
        return false;
    }
    if (consumed != text.size() ||
        parsed > std::numeric_limits<uint32_t>::max()) {
        return false;
    }
    value = static_cast<uint32_t>(parsed);
    return true;
}

[[nodiscard]] inline bool parseUint8(
    const std::string& text,
    uint8_t& value) {
    uint32_t parsed = 0;
    if (!parseUint32(text, parsed) ||
        parsed > std::numeric_limits<uint8_t>::max()) {
        return false;
    }
    value = static_cast<uint8_t>(parsed);
    return true;
}

[[nodiscard]] inline bool parseDouble(
    const std::string& text,
    double& value) {
    std::size_t consumed = 0;
    try {
        value = std::stod(text, &consumed);
    } catch (const std::exception&) {
        return false;
    }
    return consumed == text.size() && std::isfinite(value);
}

template <typename Range>
inline void printRange(const Range& values) {
    std::cout << "[";
    bool first = true;
    for (const auto& value : values) {
        if (!first) {
            std::cout << ", ";
        }
        first = false;
        std::cout << value;
    }
    std::cout << "]";
}

inline std::string formatTime(
    std::chrono::system_clock::time_point time) {
    if (time == std::chrono::system_clock::time_point{}) {
        return "unset";
    }
    const auto since_epoch =
        std::chrono::duration<double>(time.time_since_epoch()).count();
    std::ostringstream output;
    output << std::fixed << std::setprecision(6) << since_epoch;
    return output.str();
}

inline void printHeader(
    const fourier::aurora_sdk::MessageHeader& header) {
    std::cout << "header.send_time: " << formatTime(header.send_time) << "\n"
              << "header.frame_id: " << header.frame_id << "\n"
              << "header.received_time: "
              << formatTime(header.received_time) << "\n";
}

inline void printResult(
    const char* operation,
    const fourier::aurora_sdk::OperationResult& result,
    std::ostream& output = std::cerr) {
    output << operation << ": code=" << result.code
           << " name="
           << fourier::aurora_sdk::resultCodeName(result.code)
           << " message=\"" << result.message << "\"\n";
}

inline void configureClient(
    fourier::aurora_sdk::AuroraClient& client,
    uint32_t domain_id,
    const std::string& user_id,
    std::vector<fourier::aurora_sdk::Endpoint> endpoints) {
    client.setDomainId(domain_id);
    client.setUserId(user_id);
    auto options = client.config();
    options.enabled_endpoints = std::move(endpoints);
    client.configure(options);
    std::cout << "domain_id=" << client.domainId()
              << " user_id=" << client.userId()
              << " (SDK-local diagnostic label)\n";
}

inline void sleepInterruptibly(std::chrono::milliseconds duration) {
    const auto deadline = std::chrono::steady_clock::now() + duration;
    while (!stopRequested()) {
        const auto now = std::chrono::steady_clock::now();
        if (now >= deadline) {
            return;
        }
        std::this_thread::sleep_for(
            std::min(std::chrono::milliseconds(50),
                     std::chrono::duration_cast<std::chrono::milliseconds>(
                         deadline - now)));
    }
}

inline int stopClient(
    fourier::aurora_sdk::AuroraClient& client,
    int exit_code) {
    try {
        client.stop();
    } catch (const std::exception& error) {
        std::cerr << "stop failed: " << error.what() << "\n";
        return kRuntimeFailure;
    }
    return exit_code;
}

struct FailureSnapshot {
    uint32_t code = fourier::aurora_sdk::result_codes::SUCCESS;
    std::string message;
    bool present = false;

    bool changed(
        const fourier::aurora_sdk::OperationResult& result) {
        const bool is_changed =
            !present || code != result.code || message != result.message;
        code = result.code;
        message = result.message;
        present = true;
        return is_changed;
    }

    void clear() {
        present = false;
    }
};

}  // namespace client_example
