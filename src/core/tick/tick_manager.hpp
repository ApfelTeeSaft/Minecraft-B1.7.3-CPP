#pragma once

#include "util/types.hpp"
#include "platform/time/clock.hpp"
#include <functional>

namespace mcserver {

// Manages deterministic server tick loop
// Beta 1.7.3 runs at 20 TPS (50ms per tick)
class TickManager {
public:
    static constexpr i64 TARGET_MS_PER_TICK = 50; // 20 TPS
    static constexpr i64 MAX_TICK_TIME_MS = 2000; // Max catchup time

    TickManager();

    void reset();
    bool should_tick(i64& ticks_to_run);

    i64 current_tick() const { return tick_count_; }
    f64 average_tick_time_ms() const { return avg_tick_time_ms_; }
    i64 last_tick_time_ms() const { return last_tick_time_ms_; }

    void tick_started();
    void tick_finished();

private:
    i64 tick_count_;
    i64 accumulated_time_ms_;
    Clock::time_point last_update_time_;
    Clock::time_point tick_start_time_;
    i64 last_tick_time_ms_;
    f64 avg_tick_time_ms_;
};

} // namespace mcserver
