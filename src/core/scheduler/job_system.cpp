#include "job_system.hpp"
#include <condition_variable>

namespace mcserver {

JobSystem::JobSystem(u32 num_threads) {
    if (num_threads == 0) {
        num_threads = Thread::hardware_concurrency();
        if (num_threads == 0) num_threads = 4;
    }

    workers_.reserve(num_threads);
}

JobSystem::~JobSystem() {
    stop();
}

void JobSystem::start() {
    if (running_) return;

    running_ = true;
    u32 count = static_cast<u32>(workers_.capacity());

    for (u32 i = 0; i < count; ++i) {
        workers_.emplace_back([this]() { worker_thread(); });
    }
}

void JobSystem::stop() {
    if (!running_) return;

    {
        std::lock_guard<std::mutex> lock(queue_mutex_);
        running_ = false;
    }

    cv_.notify_all();

    for (auto& worker : workers_) {
        if (worker.joinable()) {
            worker.join();
        }
    }

    workers_.clear();
}

void JobSystem::submit(Job job) {
    {
        std::lock_guard<std::mutex> lock(queue_mutex_);
        job_queue_.push(std::move(job));
        ++active_jobs_;
    }
    cv_.notify_one();
}

void JobSystem::wait_all() {
    while (true) {
        std::unique_lock<std::mutex> lock(queue_mutex_);
        if (job_queue_.empty() && active_jobs_ == 0) {
            break;
        }
        lock.unlock();
        std::this_thread::yield();
    }
}

void JobSystem::worker_thread() {
    while (true) {
        Job job;

        {
            std::unique_lock<std::mutex> lock(queue_mutex_);
            cv_.wait(lock, [this]() { return !running_ || !job_queue_.empty(); });

            if (!running_ && job_queue_.empty()) {
                return;
            }

            if (!job_queue_.empty()) {
                job = std::move(job_queue_.front());
                job_queue_.pop();
            }
        }

        if (job) {
            job();

            std::lock_guard<std::mutex> lock(queue_mutex_);
            --active_jobs_;
        }
    }
}

} // namespace mcserver
