#pragma once

#include "util/types.hpp"
#include <string>
#include <memory>

namespace mcserver {

// Event priority (similar to Bukkit)
enum class EventPriority : i32 {
    LOWEST = 0,
    LOW = 1,
    NORMAL = 2,
    HIGH = 3,
    HIGHEST = 4,
    MONITOR = 5  // For monitoring only, should not modify event
};

// Base class for all events
class Event {
public:
    virtual ~Event() = default;

    // Get event name (for debugging/logging)
    virtual const char* get_event_name() const = 0;

    // Check if event is cancellable
    virtual bool is_cancellable() const { return false; }

    // Cancel the event (only works if cancellable)
    virtual void set_cancelled(bool cancelled) { (void)cancelled; }

    // Check if event is cancelled
    virtual bool is_cancelled() const { return false; }
};

// Base class for cancellable events
class CancellableEvent : public Event {
public:
    bool is_cancellable() const override { return true; }

    void set_cancelled(bool cancelled) override {
        cancelled_ = cancelled;
    }

    bool is_cancelled() const override {
        return cancelled_;
    }

private:
    bool cancelled_ = false;
};

} // namespace mcserver
