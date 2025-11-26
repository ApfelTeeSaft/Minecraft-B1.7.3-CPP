#pragma once

#include "event.hpp"
#include <functional>
#include <vector>
#include <map>
#include <memory>
#include <typeindex>
#include <mutex>
#include <algorithm>

namespace mcserver {

class Plugin;

// Event handler function type
using EventHandler = std::function<void(Event&)>;

// Event listener registration info
struct EventListener {
    Plugin* plugin;
    EventPriority priority;
    EventHandler handler;
    bool ignore_cancelled;

    EventListener(Plugin* p, EventPriority prio, EventHandler h, bool ignore_cancel = false)
        : plugin(p), priority(prio), handler(std::move(h)), ignore_cancelled(ignore_cancel) {}
};

// Event manager - dispatches events to registered listeners
class EventManager {
public:
    EventManager() = default;

    // Register an event listener
    template<typename EventType>
    void register_listener(Plugin* plugin, EventPriority priority,
                          std::function<void(EventType&)> handler,
                          bool ignore_cancelled = false) {
        std::lock_guard<std::mutex> lock(mutex_);

        auto type_index = std::type_index(typeid(EventType));

        // Wrap the typed handler in a generic EventHandler
        EventHandler generic_handler = [handler](Event& event) {
            // Safe downcast
            EventType& typed_event = dynamic_cast<EventType&>(event);
            handler(typed_event);
        };

        auto& listeners = listeners_[type_index];
        listeners.emplace_back(plugin, priority, std::move(generic_handler), ignore_cancelled);

        // Sort by priority (LOWEST first, MONITOR last)
        std::sort(listeners.begin(), listeners.end(),
                 [](const EventListener& a, const EventListener& b) {
                     return static_cast<i32>(a.priority) < static_cast<i32>(b.priority);
                 });
    }

    // Dispatch an event to all registered listeners
    template<typename EventType>
    void call_event(EventType& event) {
        std::lock_guard<std::mutex> lock(mutex_);

        auto type_index = std::type_index(typeid(EventType));
        auto it = listeners_.find(type_index);

        if (it == listeners_.end()) {
            return;  // No listeners for this event type
        }

        for (auto& listener : it->second) {
            // Skip if event is cancelled and listener ignores cancelled events
            if (event.is_cancelled() && listener.ignore_cancelled) {
                continue;
            }

            try {
                listener.handler(event);
            } catch (const std::exception& e) {
                // Log error but continue dispatching
                // TODO: Add proper logging
                (void)e;
            }
        }
    }

    // Unregister all listeners for a plugin
    void unregister_plugin(Plugin* plugin);

    // Unregister all listeners
    void unregister_all();

    // Get listener count for debugging
    usize get_listener_count() const {
        std::lock_guard<std::mutex> lock(mutex_);
        usize count = 0;
        for (const auto& [type, listeners] : listeners_) {
            count += listeners.size();
        }
        return count;
    }

private:
    mutable std::mutex mutex_;
    std::map<std::type_index, std::vector<EventListener>> listeners_;
};

} // namespace mcserver
