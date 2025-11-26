#pragma once

#include "util/types.hpp"
#include "entity/item/item_entity.hpp"
#include "entity/inventory/item_stack.hpp"
#include <unordered_map>
#include <vector>
#include <functional>
#include <memory>

namespace mcserver {

class ClientSession;
class Player;
class EntityIdManager;

// Callbacks for item entity events
using ItemSpawnCallback = std::function<void(const ItemEntity* item)>;
using ItemDespawnCallback = std::function<void(i32 entity_id)>;
using ItemCollectCallback = std::function<void(i32 item_entity_id, i32 collector_entity_id)>;

// Manages all item entities in the world
class ItemEntityManager {
public:
    explicit ItemEntityManager(EntityIdManager* id_manager);
    ~ItemEntityManager();

    // Set callbacks
    void set_spawn_callback(ItemSpawnCallback callback) {
        spawn_callback_ = std::move(callback);
    }

    void set_despawn_callback(ItemDespawnCallback callback) {
        despawn_callback_ = std::move(callback);
    }

    void set_collect_callback(ItemCollectCallback callback) {
        collect_callback_ = std::move(callback);
    }

    // Spawn an item entity in the world
    i32 spawn_item(const ItemStack& item, f64 x, f64 y, f64 z,
                   f64 velocity_x = 0.0, f64 velocity_y = 0.0, f64 velocity_z = 0.0);

    // Remove an item entity
    void remove_item(i32 entity_id);

    // Get an item entity by ID
    ItemEntity* get_item(i32 entity_id);

    // Get all item entities
    std::vector<ItemEntity*> get_all_items();

    // Update all items (called every tick)
    void tick();

    // Check for item pickups by players
    void check_pickups(const std::vector<Player*>& players);

    // Get count of active items
    usize get_item_count() const { return items_.size(); }

private:
    EntityIdManager* id_manager_;
    std::unordered_map<i32, std::unique_ptr<ItemEntity>> items_;

    ItemSpawnCallback spawn_callback_;
    ItemDespawnCallback despawn_callback_;
    ItemCollectCallback collect_callback_;
};

} // namespace mcserver
