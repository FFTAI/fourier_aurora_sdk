#pragma once

#include <array>
#include <atomic>
#include <cerrno>
#include <chrono>
#include <cstdint>
#include <csignal>
#include <cstdlib>
#include <ctime>
#include <iomanip>
#include <iostream>
#include <sstream>
#include <string>
#include <thread>
#include <utility>
#include <vector>

#include "fourier_aurora_sdk/aurora_client.hpp"
#include "fourier_aurora_sdk/types.hpp"

#if defined(__unix__) || defined(__APPLE__)
#include <sys/select.h>
#include <unistd.h>
#endif

namespace example {

inline volatile std::sig_atomic_t stop_requested = 0;

inline void handleStopSignal(int) {
    stop_requested = 1;
}

inline void installStopSignalHandlers() {
    stop_requested = 0;
#if defined(__unix__) || defined(__APPLE__)
    struct sigaction action {};
    action.sa_handler = handleStopSignal;
    sigemptyset(&action.sa_mask);
    action.sa_flags = 0;
    sigaction(SIGINT, &action, nullptr);
    sigaction(SIGTERM, &action, nullptr);
#else
    std::signal(SIGINT, handleStopSignal);
    std::signal(SIGTERM, handleStopSignal);
#endif
}

inline bool stopRequested() {
    return stop_requested != 0;
}

struct Config {
    uint32_t domain_id = 123;
    std::string user_id;
};

inline Config parseConfig(int argc, char** argv, std::string default_user_id) {
    Config config;
    config.user_id = std::move(default_user_id);
    if (argc > 1) {
        config.domain_id = static_cast<uint32_t>(std::stoul(argv[1]));
    }
    if (argc > 2) {
        config.user_id = argv[2];
    }
    return config;
}

inline void printUsage(const char* program) {
    std::cout << "Usage: " << program << " [domain_id] [user_id]\n";
}

inline void printVector(const std::vector<double>& values) {
    std::cout << "[";
    for (size_t i = 0; i < values.size(); ++i) {
        if (i > 0) {
            std::cout << ", ";
        }
        std::cout << values[i];
    }
    std::cout << "]";
}

inline void printArray3(const std::array<double, 3>& values) {
    std::cout << "[" << values[0] << ", " << values[1] << ", " << values[2] << "]";
}

inline std::string formatDateTime(std::chrono::system_clock::time_point time) {
    if (time == std::chrono::system_clock::time_point{}) {
        return "unset";
    }

    const auto time_t_value = std::chrono::system_clock::to_time_t(time);
    const auto seconds = std::chrono::system_clock::from_time_t(time_t_value);
    const auto milliseconds =
        std::chrono::duration_cast<std::chrono::milliseconds>(time - seconds).count();

    std::tm tm_value{};
    localtime_r(&time_t_value, &tm_value);

    std::ostringstream stream;
    stream << std::put_time(&tm_value, "%Y-%m-%d %H:%M:%S")
           << "." << std::setw(3) << std::setfill('0') << milliseconds;
    return stream.str();
}

inline void printHeader(const fourier::aurora_sdk::MessageHeader& header) {
    std::cout << "header.send_time: " << formatDateTime(header.send_time) << "\n"
              << "header.frame_id: " << header.frame_id << "\n"
              << "header.received_time: " << formatDateTime(header.received_time) << "\n";
}

inline void printServiceResult(const fourier::aurora_sdk::ServiceCallResult& result) {
    std::cout << "result_code=" << result.result_code
              << " success=" << (result.success() ? "true" : "false")
              << " timed_out=" << (result.timed_out ? "true" : "false")
              << " transport_error=" << (result.transport_error ? "true" : "false");
    if (!result.message.empty()) {
        std::cout << " message=\"" << result.message << "\"";
    }
    std::cout << "\n";
}

inline bool waitForEnter(const std::string& prompt) {
    std::cout << prompt;
    std::cout.flush();

#if defined(__unix__) || defined(__APPLE__)
    while (!stopRequested()) {
        fd_set read_fds;
        FD_ZERO(&read_fds);
        FD_SET(STDIN_FILENO, &read_fds);
        timeval timeout {};
        timeout.tv_usec = 100000;

        const int ready = select(STDIN_FILENO + 1, &read_fds, nullptr, nullptr, &timeout);
        if (ready > 0 && FD_ISSET(STDIN_FILENO, &read_fds)) {
            std::string line;
            return static_cast<bool>(std::getline(std::cin, line)) && !stopRequested();
        }
        if (ready < 0 && errno != EINTR) {
            return false;
        }
    }
    return false;
#else
    std::string line;
    return static_cast<bool>(std::getline(std::cin, line)) && !stopRequested();
#endif
}

inline bool closeClientWithin(
    fourier::aurora_sdk::AuroraClient& client,
    std::chrono::milliseconds timeout = std::chrono::seconds(2),
    std::chrono::milliseconds settle_time = std::chrono::milliseconds(500)) {
    std::atomic<bool> done{false};
    std::thread closer([&] {
        client.close();
        std::this_thread::sleep_for(settle_time);
        done.store(true, std::memory_order_release);
    });

    const auto deadline = std::chrono::steady_clock::now() + timeout + settle_time;
    while (!done.load(std::memory_order_acquire) &&
           std::chrono::steady_clock::now() < deadline) {
        std::this_thread::sleep_for(std::chrono::milliseconds(10));
    }

    if (done.load(std::memory_order_acquire)) {
        closer.join();
        return true;
    }

    closer.detach();
    return false;
}

inline void closeClientOrForceExit(
    fourier::aurora_sdk::AuroraClient* client,
    int exit_code) {
    if (client != nullptr && !closeClientWithin(*client)) {
        std::cerr << "AuroraClient close timed out; forcing process exit.\n";
        std::cerr.flush();
        std::cout.flush();
        std::_Exit(exit_code);
    }
}

}  // namespace example
