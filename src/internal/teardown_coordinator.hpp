#pragma once

#include <condition_variable>
#include <deque>
#include <exception>
#include <functional>
#include <future>
#include <mutex>
#include <thread>

namespace fourier::aurora_sdk::detail {

class TeardownCoordinator {
public:
    static TeardownCoordinator& instance();

    void ensureWorker();
    [[nodiscard]] std::shared_future<void> submit(std::function<void()> operation);
    [[nodiscard]] bool isWorkerThread() const;

    TeardownCoordinator(const TeardownCoordinator&) = delete;
    TeardownCoordinator& operator=(const TeardownCoordinator&) = delete;

private:
    struct Job {
        std::function<void()> operation;
        std::promise<void> completion;
    };

    TeardownCoordinator() = default;
    ~TeardownCoordinator();

    void startWorkerLocked();
    void workerLoop() noexcept;
    void failQueuedLocked(const std::exception_ptr& error) noexcept;

    mutable std::mutex mutex_;
    std::condition_variable wake_;
    std::deque<std::shared_ptr<Job>> jobs_;
    std::thread worker_;
    std::thread::id worker_id_;
    bool stopping_ = false;
    bool unavailable_ = false;
    std::exception_ptr unavailable_error_;
};

}  // namespace fourier::aurora_sdk::detail
