#include "tick_manager.hpp"
#include <algorithm>

namespace mcserver {

TickManager::TickManager()
    : tick_count_(0)
    , accumulated_time_ms_(0)
    , last_tick_time_ms_(0)
    , avg_tick_time_ms_(0.0) {
    reset();
}

void TickManager::reset() {
    tick_count_ = 0;
    accumulated_time_ms_ = 0;
    last_update_time_ = Clock::now();
    last_tick_time_ms_ = 0;
    avg_tick_time_ms_ = 0.0;
}

bool TickManager::should_tick(i64& ticks_to_run) {
    auto now = Clock::now();
    i64 elapsed_ms = Clock::elapsed_ms(last_update_time_);

    // Detect time going backwards
    if (elapsed_ms < 0) {
        last_update_time_ = now;
        return false;
    }

    // Prevent runaway catchup
    if (elapsed_ms > MAX_TICK_TIME_MS) {
        elapsed_ms = MAX_TICK_TIME_MS;
    }

    accumulated_time_ms_ += elapsed_ms;
    last_update_time_ = now;

    ticks_to_run = accumulated_time_ms_ / TARGET_MS_PER_TICK;

    if (ticks_to_run > 0) {
        accumulated_time_ms_ -= ticks_to_run * TARGET_MS_PER_TICK;
        return true;
    }

    return false;
}

void TickManager::tick_started() {
    tick_start_time_ = Clock::now();
}

void TickManager::tick_finished() {
    ++tick_count_;

    last_tick_time_ms_ = Clock::elapsed_ms(tick_start_time_);

    // Moving average for tick time
    constexpr f64 ALPHA = 0.1;
    avg_tick_time_ms_ = ALPHA * static_cast<f64>(last_tick_time_ms_) +
                        (1.0 - ALPHA) * avg_tick_time_ms_;
}

} // namespace mcserver
