#pragma once

#include "entity/inventory/item_stack.hpp"
#include "util/types.hpp"
#include <vector>
#include <memory>

namespace mcserver {

class RecipeManager;

// Inventory slot container
// Beta 1.7.3 player inventory structure:
// - Slots 0-8: Hotbar (quick access bar)
// - Slots 9-35: Main inventory (3 rows of 9)
// - Slots 36-39: Armor (boots, leggings, chestplate, helmet)
// - Slots 40-43: Crafting grid (2x2)
// - Slot 44: Crafting output
class Inventory {
public:
    static constexpr i32 HOTBAR_SIZE = 9;
    static constexpr i32 MAIN_SIZE = 27;      // 3 rows x 9 columns
    static constexpr i32 ARMOR_SIZE = 4;
    static constexpr i32 CRAFTING_GRID_SIZE = 4;  // 2x2
    static constexpr i32 TOTAL_SIZE = 45;     // 9 + 27 + 4 + 4 + 1

    // Slot indices
    static constexpr i32 HOTBAR_START = 0;
    static constexpr i32 MAIN_START = 9;
    static constexpr i32 ARMOR_START = 36;
    static constexpr i32 CRAFTING_START = 40;
    static constexpr i32 CRAFTING_OUTPUT = 44;

    Inventory();

    // Get/set item stack in slot
    ItemStack* get_slot(i32 slot);
    const ItemStack* get_slot(i32 slot) const;
    void set_slot(i32 slot, std::unique_ptr<ItemStack> stack);

    // Clear slot
    void clear_slot(i32 slot);

    // Get currently held item (from hotbar)
    ItemStack* get_held_item();
    const ItemStack* get_held_item() const;

    // Get/set current hotbar slot (0-8)
    i32 get_current_slot() const { return current_slot_; }
    void set_current_slot(i32 slot);

    // Find item in inventory
    i32 find_item(i16 item_id) const;

    // Add item to inventory (returns remaining count that couldn't be added)
    i8 add_item(std::unique_ptr<ItemStack> stack);

    // Check if item can be added to inventory (has space)
    bool can_add_item(const ItemStack& stack) const;

    // Remove item from inventory (returns true if successful)
    bool remove_item(i16 item_id, i8 count);

    // Check if inventory contains item
    bool contains_item(i16 item_id, i8 count) const;

    // Crafting operations
    void update_crafting_result(RecipeManager* recipe_manager);
    ItemStack* get_crafting_result();
    const ItemStack* get_crafting_result() const;
    void take_crafting_result();  // Take the result and consume ingredients
    std::vector<ItemStack> get_crafting_grid() const;

    // Get total size
    i32 size() const { return TOTAL_SIZE; }

    // Check if slot is valid
    bool is_valid_slot(i32 slot) const {
        return slot >= 0 && slot < TOTAL_SIZE;
    }

    // Mark inventory as changed (for network sync)
    void mark_dirty() { dirty_ = true; }
    bool is_dirty() const { return dirty_; }
    void clear_dirty() { dirty_ = false; }

private:
    std::vector<std::unique_ptr<ItemStack>> slots_;
    i32 current_slot_;  // Currently selected hotbar slot (0-8)
    bool dirty_;

    // Helper to find first empty slot
    i32 find_empty_slot() const;

    // Helper to find slot with stackable item
    i32 find_stackable_slot(const ItemStack& stack) const;
};

} // namespace mcserver
