#include "arena.hpp"
#include <algorithm>
#include <cstring>

namespace mcserver {

Arena::Block::Block(usize size)
    : memory(std::make_unique<byte[]>(size))
    , size(size)
    , used(0) {}

Arena::Arena(usize initial_capacity)
    : block_size_(initial_capacity)
    , total_allocated_(0)
    , used_(0) {
    blocks_.reserve(4); // Reserve space for a few blocks
}

Arena::~Arena() = default;

Arena::Arena(Arena&& other) noexcept
    : blocks_(std::move(other.blocks_))
    , block_size_(other.block_size_)
    , total_allocated_(other.total_allocated_)
    , used_(other.used_) {
    other.total_allocated_ = 0;
    other.used_ = 0;
}

Arena& Arena::operator=(Arena&& other) noexcept {
    if (this != &other) {
        blocks_ = std::move(other.blocks_);
        block_size_ = other.block_size_;
        total_allocated_ = other.total_allocated_;
        used_ = other.used_;
        other.total_allocated_ = 0;
        other.used_ = 0;
    }
    return *this;
}

void* Arena::allocate(usize size, usize alignment) {
    // Find a block with enough space
    for (auto& block : blocks_) {
        usize current = block.used;
        usize aligned = (current + alignment - 1) & ~(alignment - 1);
        usize needed = aligned - current + size;

        if (block.used + needed <= block.size) {
            block.used = aligned + size;
            used_ += needed;
            return reinterpret_cast<void*>(block.memory.get() + aligned);
        }
    }

    // No suitable block found, allocate a new one
    usize block_size = std::max(block_size_, size + alignment);
    add_block(block_size);

    // Try again with the new block
    auto& block = blocks_.back();
    usize aligned = (block.used + alignment - 1) & ~(alignment - 1);
    block.used = aligned + size;
    used_ += aligned + size;

    return reinterpret_cast<void*>(block.memory.get() + aligned);
}

void Arena::reset() {
    for (auto& block : blocks_) {
        block.used = 0;
    }
    used_ = 0;
}

void Arena::add_block(usize min_size) {
    usize size = std::max(block_size_, min_size);
    blocks_.emplace_back(size);
    total_allocated_ += size;
}

} // namespace mcserver
