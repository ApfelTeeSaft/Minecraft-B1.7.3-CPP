#pragma once

#include "util/types.hpp"
#include <chrono>
#include <thread>

namespace mcserver {

// High-resolution clock for timing
class Clock {
public:
    using time_point = std::chrono::steady_clock::time_point;
    using duration = std::chrono::steady_clock::duration;

    // Get current time
    static time_point now() {
        return std::chrono::steady_clock::now();
    }

    // Get elapsed time in nanoseconds
    static i64 elapsed_ns(time_point start) {
        return std::chrono::duration_cast<std::chrono::nanoseconds>(now() - start).count();
    }

    // Get elapsed time in microseconds
    static i64 elapsed_us(time_point start) {
        return std::chrono::duration_cast<std::chrono::microseconds>(now() - start).count();
    }

    // Get elapsed time in milliseconds
    static i64 elapsed_ms(time_point start) {
        return std::chrono::duration_cast<std::chrono::milliseconds>(now() - start).count();
    }

    // Convert duration to milliseconds
    static i64 to_ms(duration d) {
        return std::chrono::duration_cast<std::chrono::milliseconds>(d).count();
    }

    // Convert duration to microseconds
    static i64 to_us(duration d) {
        return std::chrono::duration_cast<std::chrono::microseconds>(d).count();
    }

    // Sleep for specified milliseconds
    static void sleep_ms(i64 ms) {
        std::this_thread::sleep_for(std::chrono::milliseconds(ms));
    }

    // Get Unix timestamp in milliseconds
    static i64 unix_timestamp_ms() {
        return std::chrono::duration_cast<std::chrono::milliseconds>(
            std::chrono::system_clock::now().time_since_epoch()
        ).count();
    }
};

} // namespace mcserver
