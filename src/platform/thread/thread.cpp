#include "thread.hpp"

namespace mcserver {

Thread::~Thread() {
    if (thread_.joinable()) {
        thread_.join();
    }
}

Thread::Thread(Thread&& other) noexcept : thread_(std::move(other.thread_)) {}

Thread& Thread::operator=(Thread&& other) noexcept {
    if (this != &other) {
        if (thread_.joinable()) {
            thread_.join();
        }
        thread_ = std::move(other.thread_);
    }
    return *this;
}

void Thread::join() {
    if (thread_.joinable()) {
        thread_.join();
    }
}

void Thread::detach() {
    if (thread_.joinable()) {
        thread_.detach();
    }
}

} // namespace mcserver
