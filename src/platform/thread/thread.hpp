#pragma once

#include <thread>
#include <functional>
#include "util/types.hpp"

namespace mcserver {

// Thread wrapper
class Thread {
public:
    Thread() = default;

    template<typename Func, typename... Args>
    explicit Thread(Func&& func, Args&&... args)
        : thread_(std::forward<Func>(func), std::forward<Args>(args)...) {}

    ~Thread();

    Thread(const Thread&) = delete;
    Thread& operator=(const Thread&) = delete;

    Thread(Thread&& other) noexcept;
    Thread& operator=(Thread&& other) noexcept;

    void join();
    void detach();

    bool joinable() const { return thread_.joinable(); }

    std::thread::id get_id() const { return thread_.get_id(); }

    static u32 hardware_concurrency() {
        return std::thread::hardware_concurrency();
    }

private:
    std::thread thread_;
};

} // namespace mcserver
