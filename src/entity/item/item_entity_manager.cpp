#include "item_entity_manager.hpp"
#include "entity/player.hpp"
#include "entity/entity_id_manager.hpp"
#include "entity/inventory/inventory.hpp"
#include "util/log/logger.hpp"

namespace mcserver {

ItemEntityManager::ItemEntityManager(EntityIdManager* id_manager)
    : id_manager_(id_manager) {
}

ItemEntityManager::~ItemEntityManager() {
    // Free all entity IDs on shutdown
    for (const auto& [eid, item] : items_) {
        if (id_manager_) {
            id_manager_->free(eid);
        }
    }
}

i32 ItemEntityManager::spawn_item(const ItemStack& item, f64 x, f64 y, f64 z,
                                   f64 velocity_x, f64 velocity_y, f64 velocity_z) {
    // Allocate entity ID
    i32 entity_id = id_manager_->allocate();

    // Create item entity
    auto item_entity = std::make_unique<ItemEntity>(entity_id, item, x, y, z);
    item_entity->set_velocity(velocity_x, velocity_y, velocity_z);

    LOG_DEBUG_CAT("Spawned item entity ID " + std::to_string(entity_id) +
                  " at (" + std::to_string(x) + ", " + std::to_string(y) + ", " + std::to_string(z) + ")",
                  LogCategory::Entity);

    // Add to tracking
    ItemEntity* item_ptr = item_entity.get();
    items_[entity_id] = std::move(item_entity);

    // Notify spawn callback
    if (spawn_callback_) {
        spawn_callback_(item_ptr);
    }

    return entity_id;
}

void ItemEntityManager::remove_item(i32 entity_id) {
    auto it = items_.find(entity_id);
    if (it != items_.end()) {
        LOG_DEBUG_CAT("Removed item entity ID " + std::to_string(entity_id),
                      LogCategory::Entity);

        // Notify despawn callback
        if (despawn_callback_) {
            despawn_callback_(entity_id);
        }

        // Free entity ID
        if (id_manager_) {
            id_manager_->free(entity_id);
        }

        items_.erase(it);
    }
}

ItemEntity* ItemEntityManager::get_item(i32 entity_id) {
    auto it = items_.find(entity_id);
    if (it != items_.end()) {
        return it->second.get();
    }
    return nullptr;
}

std::vector<ItemEntity*> ItemEntityManager::get_all_items() {
    std::vector<ItemEntity*> result;
    result.reserve(items_.size());
    for (auto& [eid, item] : items_) {
        result.push_back(item.get());
    }
    return result;
}

void ItemEntityManager::tick() {
    // Update all items and collect despawn list
    std::vector<i32> to_despawn;

    for (auto& [entity_id, item] : items_) {
        item->tick();

        // Check if item should despawn
        if (item->should_despawn()) {
            to_despawn.push_back(entity_id);
        }
    }

    // Remove despawned items
    for (i32 entity_id : to_despawn) {
        LOG_DEBUG_CAT("Item entity " + std::to_string(entity_id) + " despawned (age limit)",
                      LogCategory::Entity);
        remove_item(entity_id);
    }
}

void ItemEntityManager::check_pickups(const std::vector<Player*>& players) {
    std::vector<std::pair<i32, i32>> pickups;  // (item_eid, player_eid)

    // Check each item against each player
    for (auto& [entity_id, item] : items_) {
        if (!item->can_be_picked_up()) {
            continue;  // Item has pickup delay
        }

        for (Player* player : players) {
            if (!player || player->is_dead()) {
                continue;
            }

            // Check if player is within pickup range
            if (item->is_in_pickup_range(player->get_x(), player->get_y(), player->get_z(), 1.5)) {
                // Try to add item to player's inventory
                if (player->get_inventory()->can_add_item(*item->get_item())) {
                    // Create a copy of the item to add to inventory
                    auto item_copy = std::make_unique<ItemStack>(*item->get_item());
                    player->get_inventory()->add_item(std::move(item_copy));
                    pickups.push_back({entity_id, player->get_entity_id()});

                    LOG_DEBUG_CAT("Player " + player->get_username() + " picked up item entity " +
                                 std::to_string(entity_id), LogCategory::Entity);
                    break;  // Item picked up, stop checking other players
                }
            }
        }
    }

    // Process pickups (collect callback + remove items)
    for (const auto& [item_eid, player_eid] : pickups) {
        // Notify collect callback
        if (collect_callback_) {
            collect_callback_(item_eid, player_eid);
        }

        // Remove the item entity
        remove_item(item_eid);
    }
}

} // namespace mcserver
