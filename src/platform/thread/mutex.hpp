#pragma once

#include <mutex>

namespace mcserver {

// Mutex wrapper
using Mutex = std::mutex;
using RecursiveMutex = std::recursive_mutex;

template<typename T>
using LockGuard = std::lock_guard<T>;

template<typename T>
using UniqueLock = std::unique_lock<T>;

} // namespace mcserver
