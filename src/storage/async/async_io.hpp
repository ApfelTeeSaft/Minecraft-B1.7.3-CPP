#pragma once

#include "util/types.hpp"
#include "util/result.hpp"
#include "core/scheduler/job_system.hpp"
#include <functional>
#include <memory>

namespace mcserver {

// Async I/O wrapper around JobSystem
// Provides callback-based async operations for storage tasks
class AsyncIO {
public:
    // Callback types
    template<typename T>
    using Callback = std::function<void(Result<T>)>;

    using VoidCallback = std::function<void(Result<void>)>;

    explicit AsyncIO(JobSystem* job_system);
    ~AsyncIO() = default;

    // Submit async task with void result
    void submit_async(std::function<Result<void>()> task, VoidCallback callback);

    // Submit async task with typed result
    template<typename T>
    void submit_async(std::function<Result<T>()> task, Callback<T> callback) {
        if (!job_system_) {
            if (callback) {
                callback(ErrorCode::InvalidArgument);
            }
            return;
        }

        job_system_->submit([task = std::move(task), callback = std::move(callback)]() {
            auto result = task();
            if (callback) {
                callback(std::move(result));
            }
        });
    }

    // Check if async I/O is available
    bool is_available() const { return job_system_ != nullptr; }

private:
    JobSystem* job_system_;
};

} // namespace mcserver
