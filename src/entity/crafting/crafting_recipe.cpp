#include "crafting_recipe.hpp"
#include "world/chunk/chunk.hpp"
#include <algorithm>
#include <unordered_map>

namespace mcserver {

// Use BlockId from chunk.hpp
using BT = BlockId;

// ShapedRecipe implementation
ShapedRecipe::ShapedRecipe(const std::string& name, const std::vector<std::vector<i16>>& pattern,
                           i16 result_id, i32 result_count)
    : name_(name)
    , pattern_(pattern)
    , result_id_(result_id)
    , result_count_(result_count) {

    pattern_height_ = static_cast<i32>(pattern_.size());
    pattern_width_ = pattern_height_ > 0 ? static_cast<i32>(pattern_[0].size()) : 0;
}

bool ShapedRecipe::matches(const std::vector<ItemStack>& grid, i32 width, i32 height) const {
    // Try to match pattern at different offsets in the grid
    for (i32 offset_y = 0; offset_y <= height - pattern_height_; ++offset_y) {
        for (i32 offset_x = 0; offset_x <= width - pattern_width_; ++offset_x) {
            if (matches_at_offset(grid, width, height, offset_x, offset_y)) {
                return true;
            }
        }
    }
    return false;
}

bool ShapedRecipe::matches_at_offset(const std::vector<ItemStack>& grid, i32 grid_width, i32 grid_height,
                                     i32 offset_x, i32 offset_y) const {
    // Check if all cells outside the pattern are empty
    for (i32 y = 0; y < grid_height; ++y) {
        for (i32 x = 0; x < grid_width; ++x) {
            i32 grid_idx = y * grid_width + x;

            bool in_pattern = (x >= offset_x && x < offset_x + pattern_width_ &&
                              y >= offset_y && y < offset_y + pattern_height_);

            if (in_pattern) {
                i32 pattern_x = x - offset_x;
                i32 pattern_y = y - offset_y;
                i16 required = pattern_[pattern_y][pattern_x];

                // Check if the item matches the pattern
                if (required == 0) {
                    // Must be empty
                    if (grid[grid_idx].get_item_id() != 0) {
                        return false;
                    }
                } else if (required == -1) {
                    // Any item is fine
                    if (grid[grid_idx].get_item_id() == 0) {
                        return false;
                    }
                } else {
                    // Must match specific item
                    if (grid[grid_idx].get_item_id() != required) {
                        return false;
                    }
                }
            } else {
                // Outside pattern - must be empty
                if (grid[grid_idx].get_item_id() != 0) {
                    return false;
                }
            }
        }
    }

    return true;
}

ItemStack ShapedRecipe::get_result() const {
    return ItemStack(result_id_, static_cast<i8>(result_count_));
}

// ShapelessRecipe implementation
ShapelessRecipe::ShapelessRecipe(const std::string& name, const std::vector<i16>& ingredients,
                                 i16 result_id, i32 result_count)
    : name_(name)
    , ingredients_(ingredients)
    , result_id_(result_id)
    , result_count_(result_count) {
}

bool ShapelessRecipe::matches(const std::vector<ItemStack>& grid, i32 width, i32 height) const {
    (void)width;  // Unused for shapeless recipes
    (void)height;

    // Count items in grid
    std::unordered_map<i16, i32> grid_counts;
    for (const auto& stack : grid) {
        if (stack.get_item_id() != 0) {
            grid_counts[stack.get_item_id()]++;
        }
    }

    // Count required ingredients
    std::unordered_map<i16, i32> ingredient_counts;
    for (i16 ingredient : ingredients_) {
        ingredient_counts[ingredient]++;
    }

    // Check if grid has exactly the required ingredients
    return grid_counts == ingredient_counts;
}

ItemStack ShapelessRecipe::get_result() const {
    return ItemStack(result_id_, static_cast<i8>(result_count_));
}

// RecipeManager implementation
RecipeManager::RecipeManager() {
    register_default_recipes();
}

const CraftingRecipe* RecipeManager::find_recipe(const std::vector<ItemStack>& grid,
                                                 i32 width, i32 height) const {
    for (const auto& recipe : recipes_) {
        if (recipe->matches(grid, width, height)) {
            return recipe.get();
        }
    }
    return nullptr;
}

void RecipeManager::add_recipe(std::unique_ptr<CraftingRecipe> recipe) {
    recipes_.push_back(std::move(recipe));
}

void RecipeManager::add_shaped_recipe(const std::string& name,
                                      const std::vector<std::vector<i16>>& pattern,
                                      i16 result_id, i32 result_count) {
    recipes_.push_back(std::make_unique<ShapedRecipe>(name, pattern, result_id, result_count));
}

void RecipeManager::add_shapeless_recipe(const std::string& name,
                                         const std::vector<i16>& ingredients,
                                         i16 result_id, i32 result_count) {
    recipes_.push_back(std::make_unique<ShapelessRecipe>(name, ingredients, result_id, result_count));
}

void RecipeManager::register_default_recipes() {
    // Planks from logs (shapeless)
    add_shapeless_recipe("planks_from_log", {static_cast<i16>(BT::Wood)},
                        static_cast<i16>(BT::WoodPlanks), 4);

    // Sticks from planks (2x1 shaped)
    add_shaped_recipe("sticks", {
        {static_cast<i16>(BT::WoodPlanks)},
        {static_cast<i16>(BT::WoodPlanks)}
    }, 280, 4);  // Item ID 280 = Stick

    // Crafting table (2x2 shaped)
    add_shaped_recipe("crafting_table", {
        {static_cast<i16>(BT::WoodPlanks), static_cast<i16>(BT::WoodPlanks)},
        {static_cast<i16>(BT::WoodPlanks), static_cast<i16>(BT::WoodPlanks)}
    }, 58, 1);  // Block ID 58 = Crafting Table

    // Torches (shapeless - 1 coal item + 1 stick)
    add_shapeless_recipe("torch", {263, 280},  // 263 = Coal (item), 280 = Stick
                        50, 4);  // Block ID 50 = Torch

    // Wool from string (2x2 shaped)
    add_shaped_recipe("wool_from_string", {
        {287, 287},  // 287 = String
        {287, 287}
    }, 35, 1);  // Block ID 35 = Wool

    // Wooden tools (require 3x3 crafting table, but we can add 2x2 recipes)

    // Note: Most tools require 3x3 crafting, but we can add some simple 2x2 recipes
    // For full crafting support, players need to craft a crafting table first
}

} // namespace mcserver
