#pragma once

#include "util/types.hpp"
#include <vector>
#include <mutex>

namespace mcserver {

// Manages entity IDs with recycling
// Thread-safe entity ID allocation and deallocation
class EntityIdManager {
public:
    EntityIdManager() : next_id_(1) {}

    // Allocate a new entity ID
    i32 allocate() {
        std::lock_guard<std::mutex> lock(mutex_);

        // Try to reuse a freed ID first
        if (!freed_ids_.empty()) {
            i32 id = freed_ids_.back();
            freed_ids_.pop_back();
            return id;
        }

        // Otherwise, allocate a new ID
        return next_id_++;
    }

    // Free an entity ID for reuse
    void free(i32 id) {
        std::lock_guard<std::mutex> lock(mutex_);

        // Add to freed IDs list for reuse
        // Only add if it's not already in the list (prevent duplicates)
        if (id > 0 && id < next_id_) {
            freed_ids_.push_back(id);
        }
    }

    // Reset the manager (useful for tests or server restart)
    void reset() {
        std::lock_guard<std::mutex> lock(mutex_);
        next_id_ = 1;
        freed_ids_.clear();
    }

    // Get the total number of IDs allocated (including freed ones)
    i32 get_total_allocated() const {
        std::lock_guard<std::mutex> lock(mutex_);
        return next_id_ - 1;
    }

    // Get the number of active (not freed) IDs
    i32 get_active_count() const {
        std::lock_guard<std::mutex> lock(mutex_);
        return (next_id_ - 1) - static_cast<i32>(freed_ids_.size());
    }

private:
    mutable std::mutex mutex_;
    i32 next_id_;
    std::vector<i32> freed_ids_;
};

} // namespace mcserver
