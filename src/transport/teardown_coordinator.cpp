#include "internal/teardown_coordinator.hpp"

#include <utility>

#include "fourier_aurora_sdk/errors.hpp"

namespace fourier::aurora_sdk::detail {

TeardownCoordinator& TeardownCoordinator::instance() {
    static TeardownCoordinator coordinator;
    return coordinator;
}

TeardownCoordinator::~TeardownCoordinator() {
    {
        std::lock_guard<std::mutex> lock(mutex_);
        stopping_ = true;
    }
    wake_.notify_all();
    if (worker_.joinable()) {
        worker_.join();
    }
}

void TeardownCoordinator::startWorkerLocked() {
    if (worker_.joinable()) {
        return;
    }
    if (stopping_ || unavailable_) {
        throw ConnectionError("transport teardown coordinator is unavailable");
    }
    try {
        worker_ = std::thread([this] { workerLoop(); });
    } catch (const std::exception& error) {
        unavailable_ = true;
        unavailable_error_ =
            std::make_exception_ptr(ConnectionError(
                std::string("failed to start transport teardown coordinator: ") +
                error.what()));
        throw ConnectionError(
            std::string("failed to start transport teardown coordinator: ") +
            error.what());
    } catch (...) {
        unavailable_ = true;
        unavailable_error_ = std::make_exception_ptr(
            ConnectionError("failed to start transport teardown coordinator"));
        throw ConnectionError("failed to start transport teardown coordinator");
    }
}

void TeardownCoordinator::ensureWorker() {
    std::lock_guard<std::mutex> lock(mutex_);
    startWorkerLocked();
}

std::shared_future<void> TeardownCoordinator::submit(
    std::function<void()> operation) {
    if (!operation) {
        throw InvalidArgumentError("teardown operation must not be empty");
    }

    std::lock_guard<std::mutex> lock(mutex_);
    if (worker_id_ == std::this_thread::get_id()) {
        throw ConnectionError(
            "teardown coordinator worker cannot submit recursively");
    }
    startWorkerLocked();
    if (unavailable_) {
        if (unavailable_error_) {
            std::rethrow_exception(unavailable_error_);
        }
        throw ConnectionError("transport teardown coordinator is unavailable");
    }

    auto job = std::make_shared<Job>();
    job->operation = std::move(operation);
    auto completion = job->completion.get_future().share();
    jobs_.push_back(std::move(job));
    wake_.notify_one();
    return completion;
}

bool TeardownCoordinator::isWorkerThread() const {
    std::lock_guard<std::mutex> lock(mutex_);
    return worker_id_ == std::this_thread::get_id();
}

void TeardownCoordinator::failQueuedLocked(
    const std::exception_ptr& error) noexcept {
    while (!jobs_.empty()) {
        auto job = std::move(jobs_.front());
        jobs_.pop_front();
        try {
            job->completion.set_exception(error);
        } catch (...) {
        }
    }
}

void TeardownCoordinator::workerLoop() noexcept {
    try {
        {
            std::lock_guard<std::mutex> lock(mutex_);
            worker_id_ = std::this_thread::get_id();
        }

        for (;;) {
            std::shared_ptr<Job> job;
            {
                std::unique_lock<std::mutex> lock(mutex_);
                wake_.wait(lock, [this] { return stopping_ || !jobs_.empty(); });
                if (stopping_ && jobs_.empty()) {
                    worker_id_ = {};
                    return;
                }
                job = std::move(jobs_.front());
                jobs_.pop_front();
            }

            try {
                job->operation();
                job->completion.set_value();
            } catch (...) {
                try {
                    job->completion.set_exception(std::current_exception());
                } catch (...) {
                }
            }
        }
    } catch (...) {
        const auto error = std::make_exception_ptr(
            ConnectionError("transport teardown coordinator worker failed"));
        std::lock_guard<std::mutex> lock(mutex_);
        unavailable_ = true;
        unavailable_error_ = error;
        worker_id_ = {};
        failQueuedLocked(error);
    }
}

}  // namespace fourier::aurora_sdk::detail
