#pragma once

#include <memory>
#include <vector>
#include "util/types.hpp"

namespace mcserver {

// Object pool for reusing fixed-size objects
// Thread-safe version would use mutex or lock-free stack
template<typename T>
class Pool {
public:
    explicit Pool(usize initial_capacity = 64);
    ~Pool();

    Pool(const Pool&) = delete;
    Pool& operator=(const Pool&) = delete;

    // Acquire an object from the pool (constructs if needed)
    template<typename... Args>
    T* acquire(Args&&... args);

    // Release an object back to the pool
    void release(T* object);

    // Get pool statistics
    usize capacity() const { return objects_.size(); }
    usize available() const { return free_list_.size(); }

private:
    std::vector<std::unique_ptr<T>> objects_;
    std::vector<T*> free_list_;
};

template<typename T>
Pool<T>::Pool(usize initial_capacity) {
    objects_.reserve(initial_capacity);
    free_list_.reserve(initial_capacity);
}

template<typename T>
Pool<T>::~Pool() = default;

template<typename T>
template<typename... Args>
T* Pool<T>::acquire(Args&&... args) {
    if (free_list_.empty()) {
        // No free objects, allocate a new one
        auto obj = std::make_unique<T>(std::forward<Args>(args)...);
        T* ptr = obj.get();
        objects_.push_back(std::move(obj));
        return ptr;
    }

    // Reuse from free list
    T* obj = free_list_.back();
    free_list_.pop_back();

    // Placement new to reinitialize
    obj->~T();
    new (obj) T(std::forward<Args>(args)...);

    return obj;
}

template<typename T>
void Pool<T>::release(T* object) {
    if (object) {
        free_list_.push_back(object);
    }
}

} // namespace mcserver
