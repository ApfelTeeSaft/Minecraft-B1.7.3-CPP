#include "entity/inventory/item_stack.hpp"
#include <algorithm>

namespace mcserver {

ItemStack::ItemStack()
    : item_id_(-1), count_(0), damage_(0) {
}

ItemStack::ItemStack(i16 item_id, i8 count, i16 damage)
    : item_id_(item_id), count_(count), damage_(damage) {
}

bool ItemStack::can_stack_with(const ItemStack& other) const {
    if (is_empty() || other.is_empty()) {
        return false;
    }
    return item_id_ == other.item_id_ && damage_ == other.damage_;
}

i8 ItemStack::get_max_stack_size() const {
    // Implement proper max stack sizes per item type (Beta 1.7.3)

    // Items that stack to 16
    if (item_id_ == 332 ||  // Snowball
        item_id_ == 344 ||  // Egg
        item_id_ == 368) {  // Ender Pearl
        return 16;
    }

    // Items that stack to 1 (tools, weapons, armor, special items)
    // Tools (256-279)
    if ((item_id_ >= 256 && item_id_ <= 259) ||  // Shovels (iron, wood, stone, diamond, gold would be 256-259)
        (item_id_ >= 267 && item_id_ <= 279)) {  // Swords, pickaxes, axes, hoes
        return 1;
    }

    // Armor (298-317)
    if (item_id_ >= 298 && item_id_ <= 317) {
        return 1;
    }

    // Specific items that don't stack
    if (item_id_ == 325 ||  // Bucket (empty)
        item_id_ == 326 ||  // Water bucket
        item_id_ == 327 ||  // Lava bucket
        item_id_ == 335 ||  // Milk bucket
        item_id_ == 323 ||  // Sign
        item_id_ == 324 ||  // Door (wood)
        item_id_ == 330 ||  // Door (iron)
        item_id_ == 342 ||  // Minecart
        item_id_ == 343 ||  // Boat
        item_id_ == 345 ||  // Compass
        item_id_ == 346 ||  // Clock
        item_id_ == 347 ||  // Bed
        item_id_ == 354 ||  // Cake
        item_id_ == 355 ||  // Fishing rod
        item_id_ == 259) {  // Flint and steel
        return 1;
    }

    // Most items and blocks stack to 64
    return 64;
}

void ItemStack::decrease_count(i8 amount) {
    count_ -= amount;
    if (count_ < 0) {
        count_ = 0;
    }
    if (count_ == 0) {
        item_id_ = -1;  // Mark as empty
    }
}

void ItemStack::increase_count(i8 amount) {
    count_ += amount;
    i8 max_size = get_max_stack_size();
    if (count_ > max_size) {
        count_ = max_size;
    }
}

std::unique_ptr<ItemStack> ItemStack::split(i8 amount) {
    if (amount <= 0 || is_empty()) {
        return std::make_unique<ItemStack>();
    }

    i8 split_amount = std::min(amount, count_);
    decrease_count(split_amount);

    return std::make_unique<ItemStack>(item_id_, split_amount, damage_);
}

std::unique_ptr<ItemStack> ItemStack::clone() const {
    return std::make_unique<ItemStack>(item_id_, count_, damage_);
}

} // namespace mcserver
