#pragma once

#include "entity/inventory/item_stack.hpp"
#include "util/types.hpp"
#include <vector>
#include <memory>
#include <string>

namespace mcserver {

// Crafting recipe types
enum class RecipeType {
    Shaped,     // Pattern must match exactly (e.g., pickaxe)
    Shapeless   // Items can be in any position (e.g., flint and steel)
};

// Base crafting recipe class
class CraftingRecipe {
public:
    virtual ~CraftingRecipe() = default;

    // Check if the crafting grid matches this recipe
    virtual bool matches(const std::vector<ItemStack>& grid, i32 width, i32 height) const = 0;

    // Get the result of this recipe
    virtual ItemStack get_result() const = 0;

    // Get the recipe type
    virtual RecipeType get_type() const = 0;

    // Get recipe name (for debugging)
    virtual std::string get_name() const = 0;
};

// Shaped recipe (pattern must match)
class ShapedRecipe : public CraftingRecipe {
public:
    ShapedRecipe(const std::string& name, const std::vector<std::vector<i16>>& pattern,
                 i16 result_id, i32 result_count = 1);

    bool matches(const std::vector<ItemStack>& grid, i32 width, i32 height) const override;
    ItemStack get_result() const override;
    RecipeType get_type() const override { return RecipeType::Shaped; }
    std::string get_name() const override { return name_; }

private:
    std::string name_;
    std::vector<std::vector<i16>> pattern_;  // -1 = any item, 0 = empty, >0 = specific item
    i16 result_id_;
    i32 result_count_;
    i32 pattern_width_;
    i32 pattern_height_;

    // Check if pattern matches at a specific offset in the grid
    bool matches_at_offset(const std::vector<ItemStack>& grid, i32 grid_width, i32 grid_height,
                          i32 offset_x, i32 offset_y) const;
};

// Shapeless recipe (items can be in any position)
class ShapelessRecipe : public CraftingRecipe {
public:
    ShapelessRecipe(const std::string& name, const std::vector<i16>& ingredients,
                   i16 result_id, i32 result_count = 1);

    bool matches(const std::vector<ItemStack>& grid, i32 width, i32 height) const override;
    ItemStack get_result() const override;
    RecipeType get_type() const override { return RecipeType::Shapeless; }
    std::string get_name() const override { return name_; }

private:
    std::string name_;
    std::vector<i16> ingredients_;
    i16 result_id_;
    i32 result_count_;
};

// Recipe manager - holds all crafting recipes
class RecipeManager {
public:
    RecipeManager();

    // Find a matching recipe for the given crafting grid
    const CraftingRecipe* find_recipe(const std::vector<ItemStack>& grid, i32 width, i32 height) const;

    // Add a custom recipe
    void add_recipe(std::unique_ptr<CraftingRecipe> recipe);

    // Get all recipes (for recipe book, etc.)
    const std::vector<std::unique_ptr<CraftingRecipe>>& get_all_recipes() const {
        return recipes_;
    }

private:
    std::vector<std::unique_ptr<CraftingRecipe>> recipes_;

    // Register default Beta 1.7.3 recipes
    void register_default_recipes();

    // Helper methods for adding recipes
    void add_shaped_recipe(const std::string& name, const std::vector<std::vector<i16>>& pattern,
                          i16 result_id, i32 result_count = 1);
    void add_shapeless_recipe(const std::string& name, const std::vector<i16>& ingredients,
                             i16 result_id, i32 result_count = 1);
};

} // namespace mcserver
