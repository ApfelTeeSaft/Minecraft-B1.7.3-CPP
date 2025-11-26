#pragma once

#include "util/types.hpp"
#include <memory>

namespace mcserver {

// Represents a stack of items in inventory
// Beta 1.7.3 item stack structure
class ItemStack {
public:
    ItemStack();
    ItemStack(i16 item_id, i8 count, i16 damage = 0);

    // Item properties
    i16 get_item_id() const { return item_id_; }
    i8 get_count() const { return count_; }
    i16 get_damage() const { return damage_; }

    void set_item_id(i16 id) { item_id_ = id; }
    void set_count(i8 count) { count_ = count; }
    void set_damage(i16 damage) { damage_ = damage; }

    // Stack operations
    bool is_empty() const { return item_id_ < 0 || count_ <= 0; }
    bool can_stack_with(const ItemStack& other) const;
    i8 get_max_stack_size() const;

    // Modify stack
    void decrease_count(i8 amount);
    void increase_count(i8 amount);

    // Split stack (returns new stack with split amount)
    std::unique_ptr<ItemStack> split(i8 amount);

    // Clone
    std::unique_ptr<ItemStack> clone() const;

private:
    i16 item_id_;  // Item/block ID (-1 = empty)
    i8 count_;     // Number of items (0-64 typically)
    i16 damage_;   // Durability damage or metadata
};

} // namespace mcserver
