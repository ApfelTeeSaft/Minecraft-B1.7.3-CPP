#pragma once

#include <functional>
#include <vector>
#include <queue>
#include <mutex>
#include <condition_variable>
#include "platform/thread/thread.hpp"
#include "util/types.hpp"

namespace mcserver {

// Simple job system for parallel tasks
// Step 1: Basic implementation, Step 2 will add work-stealing
class JobSystem {
public:
    using Job = std::function<void()>;

    explicit JobSystem(u32 num_threads = 0);
    ~JobSystem();

    void start();
    void stop();

    void submit(Job job);
    void wait_all();

    u32 thread_count() const { return static_cast<u32>(workers_.size()); }

private:
    void worker_thread();

    std::vector<Thread> workers_;
    std::queue<Job> job_queue_;
    std::mutex queue_mutex_;
    std::condition_variable cv_;
    bool running_ = false;
    u32 active_jobs_ = 0;
};

} // namespace mcserver
