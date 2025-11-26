#pragma once

#include <memory>
#include <vector>
#include "util/types.hpp"

namespace mcserver {

// Arena allocator for temporary allocations that are freed all at once
// Useful for per-tick temporary allocations
class Arena {
public:
    explicit Arena(usize initial_capacity = 1024 * 1024); // 1MB default
    ~Arena();

    Arena(const Arena&) = delete;
    Arena& operator=(const Arena&) = delete;

    Arena(Arena&&) noexcept;
    Arena& operator=(Arena&&) noexcept;

    // Allocate memory from the arena
    void* allocate(usize size, usize alignment = alignof(std::max_align_t));

    // Allocate and construct object
    template<typename T, typename... Args>
    T* create(Args&&... args) {
        void* mem = allocate(sizeof(T), alignof(T));
        return new (mem) T(std::forward<Args>(args)...);
    }

    // Reset the arena (frees all allocations)
    void reset();

    // Get total allocated size
    usize total_allocated() const { return total_allocated_; }

    // Get used size
    usize used() const { return used_; }

private:
    struct Block {
        std::unique_ptr<byte[]> memory;
        usize size;
        usize used;

        Block(usize size);
    };

    std::vector<Block> blocks_;
    usize block_size_;
    usize total_allocated_;
    usize used_;

    void add_block(usize min_size);
};

} // namespace mcserver
