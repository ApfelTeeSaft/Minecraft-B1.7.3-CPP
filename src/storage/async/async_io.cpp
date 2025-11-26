#include "async_io.hpp"
#include "core/scheduler/job_system.hpp"

namespace mcserver {

AsyncIO::AsyncIO(JobSystem* job_system)
    : job_system_(job_system) {
}

void AsyncIO::submit_async(std::function<Result<void>()> task, VoidCallback callback) {
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

} // namespace mcserver
