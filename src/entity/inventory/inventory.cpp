#include "entity/inventory/inventory.hpp"
#include "entity/crafting/crafting_recipe.hpp"
#include <algorithm>

namespace mcserver {

Inventory::Inventory()
    : current_slot_(0), dirty_(false) {
    // Initialize all slots as empty
    slots_.reserve(TOTAL_SIZE);
    for (i32 i = 0; i < TOTAL_SIZE; ++i) {
        slots_.push_back(std::make_unique<ItemStack>());
    }
}

ItemStack* Inventory::get_slot(i32 slot) {
    if (!is_valid_slot(slot)) {
        return nullptr;
    }
    return slots_[slot].get();
}

const ItemStack* Inventory::get_slot(i32 slot) const {
    if (!is_valid_slot(slot)) {
        return nullptr;
    }
    return slots_[slot].get();
}

void Inventory::set_slot(i32 slot, std::unique_ptr<ItemStack> stack) {
    if (!is_valid_slot(slot)) {
        return;
    }
    slots_[slot] = std::move(stack);
    mark_dirty();
}

void Inventory::clear_slot(i32 slot) {
    if (!is_valid_slot(slot)) {
        return;
    }
    slots_[slot] = std::make_unique<ItemStack>();
    mark_dirty();
}

ItemStack* Inventory::get_held_item() {
    if (current_slot_ < 0 || current_slot_ >= HOTBAR_SIZE) {
        return nullptr;
    }
    return slots_[current_slot_].get();
}

const ItemStack* Inventory::get_held_item() const {
    if (current_slot_ < 0 || current_slot_ >= HOTBAR_SIZE) {
        return nullptr;
    }
    return slots_[current_slot_].get();
}

void Inventory::set_current_slot(i32 slot) {
    if (slot >= 0 && slot < HOTBAR_SIZE) {
        current_slot_ = slot;
        mark_dirty();
    }
}

i32 Inventory::find_item(i16 item_id) const {
    for (i32 i = 0; i < TOTAL_SIZE; ++i) {
        const auto* stack = slots_[i].get();
        if (stack && !stack->is_empty() && stack->get_item_id() == item_id) {
            return i;
        }
    }
    return -1;
}

i32 Inventory::find_empty_slot() const {
    // Only search hotbar (0-8) and main inventory (9-35), not armor/crafting slots
    // This ensures items are placed in visible slots that get synced to the client
    for (i32 i = 0; i < ARMOR_START; ++i) {  // ARMOR_START = 36
        const auto* stack = slots_[i].get();
        if (stack && stack->is_empty()) {
            return i;
        }
    }
    return -1;
}

i32 Inventory::find_stackable_slot(const ItemStack& stack) const {
    // Only search hotbar (0-8) and main inventory (9-35), not armor/crafting slots
    // This ensures items are placed in visible slots that get synced to the client
    for (i32 i = 0; i < ARMOR_START; ++i) {  // ARMOR_START = 36
        const auto* existing = slots_[i].get();
        if (existing && !existing->is_empty() && existing->can_stack_with(stack)) {
            // Check if there's room to add more
            if (existing->get_count() < existing->get_max_stack_size()) {
                return i;
            }
        }
    }
    return -1;
}

i8 Inventory::add_item(std::unique_ptr<ItemStack> stack) {
    if (!stack || stack->is_empty()) {
        return 0;
    }

    i8 remaining = stack->get_count();
    i16 item_id = stack->get_item_id();
    i16 damage = stack->get_damage();

    // First, try to stack with existing items
    while (remaining > 0) {
        i32 slot = find_stackable_slot(*stack);
        if (slot < 0) {
            break;
        }

        auto* existing = slots_[slot].get();
        i8 max_size = existing->get_max_stack_size();
        i8 current = existing->get_count();
        i8 can_add = std::min(remaining, static_cast<i8>(max_size - current));

        existing->increase_count(can_add);
        remaining -= can_add;
        mark_dirty();
    }

    // Then, try to add to empty slots
    while (remaining > 0) {
        i32 slot = find_empty_slot();
        if (slot < 0) {
            break;  // No more room
        }

        i8 max_size = stack->get_max_stack_size();
        i8 to_add = std::min(remaining, max_size);

        slots_[slot] = std::make_unique<ItemStack>(item_id, to_add, damage);
        remaining -= to_add;
        mark_dirty();
    }

    return remaining;
}

bool Inventory::can_add_item(const ItemStack& stack) const {
    if (stack.is_empty()) {
        return true;  // Empty stack can always be "added"
    }

    i8 remaining = stack.get_count();

    // Check if we can stack with existing items
    for (const auto& slot : slots_) {
        if (!slot || slot->is_empty()) {
            continue;
        }

        if (slot->get_item_id() == stack.get_item_id() &&
            slot->get_damage() == stack.get_damage()) {
            i8 max_size = slot->get_max_stack_size();
            i8 current = slot->get_count();
            i8 can_add = max_size - current;

            if (can_add > 0) {
                remaining -= can_add;
                if (remaining <= 0) {
                    return true;  // Can fit entirely in existing stacks
                }
            }
        }
    }

    // Check if we have empty slots
    i32 empty_slots = 0;
    for (const auto& slot : slots_) {
        if (!slot || slot->is_empty()) {
            empty_slots++;
        }
    }

    // Calculate how many items can fit in empty slots
    i8 max_size = stack.get_max_stack_size();
    i8 can_fit_in_empty = static_cast<i8>(empty_slots * max_size);

    return remaining <= can_fit_in_empty;
}

bool Inventory::remove_item(i16 item_id, i8 count) {
    if (count <= 0) {
        return true;
    }

    // First, check if we have enough
    if (!contains_item(item_id, count)) {
        return false;
    }

    i8 remaining = count;

    // Remove from slots
    for (i32 i = 0; i < TOTAL_SIZE && remaining > 0; ++i) {
        auto* stack = slots_[i].get();
        if (stack && !stack->is_empty() && stack->get_item_id() == item_id) {
            i8 to_remove = std::min(remaining, stack->get_count());
            stack->decrease_count(to_remove);
            remaining -= to_remove;
            mark_dirty();
        }
    }

    return remaining == 0;
}

bool Inventory::contains_item(i16 item_id, i8 count) const {
    i8 total = 0;

    for (i32 i = 0; i < TOTAL_SIZE; ++i) {
        const auto* stack = slots_[i].get();
        if (stack && !stack->is_empty() && stack->get_item_id() == item_id) {
            total += stack->get_count();
            if (total >= count) {
                return true;
            }
        }
    }

    return total >= count;
}

void Inventory::update_crafting_result(RecipeManager* recipe_manager) {
    if (!recipe_manager) {
        clear_slot(CRAFTING_OUTPUT);
        return;
    }

    // Get crafting grid items
    std::vector<ItemStack> grid = get_crafting_grid();

    // Find matching recipe
    const CraftingRecipe* recipe = recipe_manager->find_recipe(grid, 2, 2);

    // Update output slot
    if (recipe) {
        ItemStack result = recipe->get_result();
        slots_[CRAFTING_OUTPUT] = std::make_unique<ItemStack>(result);
    } else {
        clear_slot(CRAFTING_OUTPUT);
    }
}

ItemStack* Inventory::get_crafting_result() {
    return get_slot(CRAFTING_OUTPUT);
}

const ItemStack* Inventory::get_crafting_result() const {
    return get_slot(CRAFTING_OUTPUT);
}

void Inventory::take_crafting_result() {
    // Clear the output (player took it)
    clear_slot(CRAFTING_OUTPUT);

    // Consume one item from each crafting grid slot
    for (i32 i = CRAFTING_START; i < CRAFTING_START + CRAFTING_GRID_SIZE; ++i) {
        ItemStack* stack = get_slot(i);
        if (stack && !stack->is_empty()) {
            stack->decrease_count(1);
        }
    }

    mark_dirty();
}

std::vector<ItemStack> Inventory::get_crafting_grid() const {
    std::vector<ItemStack> grid;
    grid.reserve(CRAFTING_GRID_SIZE);

    for (i32 i = CRAFTING_START; i < CRAFTING_START + CRAFTING_GRID_SIZE; ++i) {
        const ItemStack* stack = get_slot(i);
        if (stack) {
            grid.push_back(*stack);
        } else {
            grid.push_back(ItemStack());  // Empty slot
        }
    }

    return grid;
}

} // namespace mcserver
