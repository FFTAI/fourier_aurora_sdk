#include <atomic>
#include <charconv>
#include <chrono>
#include <csignal>
#include <cstdint>
#include <ctime>
#include <exception>
#include <iomanip>
#include <iostream>
#include <sstream>
#include <string>
#include <string_view>
#include <thread>

#include "fourier_aurora_sdk/aurora_client.hpp"

namespace sdk = fourier::aurora_sdk;

namespace {

volatile std::sig_atomic_t interrupted = 0;

void handleSignal(int) {
    interrupted = 1;
}

bool parseDurationSeconds(
    std::string_view text,
    std::uint32_t& duration_seconds) {
    const auto result = std::from_chars(
        text.data(), text.data() + text.size(), duration_seconds);
    return result.ec == std::errc{} &&
           result.ptr == text.data() + text.size() && duration_seconds != 0;
}

std::string formatTime(
    std::chrono::system_clock::time_point time_point) {
    const auto whole_seconds =
        std::chrono::time_point_cast<std::chrono::seconds>(time_point);
    const auto microseconds =
        std::chrono::duration_cast<std::chrono::microseconds>(
            time_point - whole_seconds)
            .count();
    const std::time_t time =
        std::chrono::system_clock::to_time_t(whole_seconds);
    std::tm local_time{};
    localtime_r(&time, &local_time);

    std::ostringstream output;
    output << std::put_time(&local_time, "%H:%M:%S") << ':'
           << std::setfill('0') << std::setw(6) << microseconds;
    return output.str();
}

void stopAfterFailure(sdk::AuroraClient& client) {
    try {
        client.stop();
    } catch (const std::exception& error) {
        std::cerr << "stop failed: " << error.what() << "\n";
    }
}

}  // namespace

int main(int argc, char** argv) {
    std::uint32_t duration_seconds = 0;
    if (argc > 2 ||
        (argc == 2 && !parseDurationSeconds(argv[1], duration_seconds))) {
        std::cerr << "Usage: " << argv[0] << " [positive_duration_seconds]\n";
        return 2;
    }

    std::signal(SIGINT, handleSignal);
    std::signal(SIGTERM, handleSignal);

    auto& client = sdk::AuroraClient::getInstance();
    std::atomic<std::uint64_t> callback_count{0};

    try {
        auto options = client.config();
        options.enabled_endpoints = {
            sdk::Endpoint::AuroraStateSubscriber,
        };
        client.setDomainId(89);
        client.configure(options);

        client.onAuroraState(
            [&callback_count](sdk::AuroraState state) {
                const auto count = ++callback_count;
                if (count % 10 != 0) {
                    return;
                }

                std::cout
                    << "AuroraState send_time="
                    << formatTime(state.header.send_time)
                    << " received_time="
                    << formatTime(state.header.received_time)
                    << " total_callbacks=" << count << "\n";
            });

        client.start();
        if (!client.waitForEndpoints(std::chrono::seconds(5))) {
            std::cerr << "AuroraState endpoint was not ready within 5 s.\n";
            client.stop();
            return interrupted ? 130 : 1;
        }

        std::cout << "Receiving AuroraState; press Ctrl+C to stop.\n";
        const auto deadline = std::chrono::steady_clock::now() +
                              std::chrono::seconds(duration_seconds);
        while (!interrupted &&
               (duration_seconds == 0 ||
                std::chrono::steady_clock::now() < deadline)) {
            std::this_thread::sleep_for(std::chrono::milliseconds(100));
        }

        client.stop();
        std::cout << "Stopped after " << callback_count.load()
                  << " AuroraState callbacks.\n";
        return interrupted ? 130 : 0;
    } catch (const std::exception& error) {
        std::cerr << "GR3 AuroraState callback example failed: "
                  << error.what() << "\n";
        stopAfterFailure(client);
        return interrupted ? 130 : 1;
    }
}
