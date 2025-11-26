#include "entity_manager.hpp"
#include "net/session/client_session.hpp"
#include "util/log/logger.hpp"
#include <cmath>

namespace mcserver {

void EntityManager::add_player(Player* player, ClientSession* session) {
    if (!player || !session) {
        return;
    }

    i32 entity_id = player->get_entity_id();
    players_[entity_id] = player;
    player_sessions_[entity_id] = session;

    // Set up health change callback for this player
    if (health_change_callback_) {
        player->set_health_change_callback(health_change_callback_);
    }

    // Set up death callback for this player
    if (death_callback_) {
        player->set_death_callback(death_callback_);
    }

    LOG_DEBUG_CAT("EntityManager: Added player " + player->get_username() +
                  " (entity ID: " + std::to_string(entity_id) + ")",
                  LogCategory::Entity);
}

void EntityManager::remove_player(i32 entity_id) {
    auto it = players_.find(entity_id);
    if (it != players_.end()) {
        LOG_DEBUG_CAT("EntityManager: Removed player entity ID " + std::to_string(entity_id),
                      LogCategory::Entity);
        players_.erase(it);
        player_sessions_.erase(entity_id);

        // Free the entity ID for reuse
        id_manager_.free(entity_id);
    }
}

Player* EntityManager::get_player(i32 entity_id) {
    auto it = players_.find(entity_id);
    if (it != players_.end()) {
        return it->second;
    }
    return nullptr;
}

std::vector<Player*> EntityManager::get_other_players(i32 exclude_entity_id) {
    std::vector<Player*> result;
    for (auto& [entity_id, player] : players_) {
        if (entity_id != exclude_entity_id) {
            result.push_back(player);
        }
    }
    return result;
}

std::vector<Player*> EntityManager::get_all_players() {
    std::vector<Player*> result;
    result.reserve(players_.size());
    for (auto& [entity_id, player] : players_) {
        result.push_back(player);
    }
    return result;
}

ClientSession* EntityManager::get_player_session(i32 entity_id) {
    auto it = player_sessions_.find(entity_id);
    if (it != player_sessions_.end()) {
        return it->second;
    }
    return nullptr;
}

void EntityManager::spawn_existing_entities_for(ClientSession* new_client) {
    if (!new_client || !spawn_player_callback_) {
        return;
    }

    const Player* new_player = new_client->get_player();
    if (!new_player) {
        return;
    }

    // Spawn all existing players to the new client
    for (auto& [entity_id, player] : players_) {
        if (entity_id != new_player->get_entity_id()) {
            // Check if in range (for now, spawn all - can add range check later)
            if (is_in_range(new_player, player)) {
                spawn_player_callback_(new_client, player);
            }
        }
    }

    LOG_DEBUG_CAT("EntityManager: Spawned " + std::to_string(players_.size() - 1) +
                  " existing entities for " + new_player->get_username(),
                  LogCategory::Entity);
}

void EntityManager::spawn_entity_for_nearby_players(Player* player, ClientSession* exclude_session) {
    if (!player || !spawn_player_callback_) {
        return;
    }

    // Spawn this player to all other connected players
    for (auto& [entity_id, session] : player_sessions_) {
        if (session != exclude_session) {
            const Player* other_player = session->get_player();
            if (other_player && other_player->get_entity_id() != player->get_entity_id()) {
                // Check if in range
                if (is_in_range(player, other_player)) {
                    spawn_player_callback_(session, player);
                }
            }
        }
    }

    LOG_DEBUG_CAT("EntityManager: Spawned " + player->get_username() +
                  " for nearby players",
                  LogCategory::Entity);
}

void EntityManager::despawn_entity_for_all(i32 entity_id) {
    if (!despawn_entity_callback_) {
        return;
    }

    // Send despawn packet to all connected players
    for (auto& [eid, session] : player_sessions_) {
        if (eid != entity_id) {
            despawn_entity_callback_(session, entity_id);
        }
    }

    LOG_DEBUG_CAT("EntityManager: Despawned entity ID " + std::to_string(entity_id) +
                  " for all players",
                  LogCategory::Entity);
}

void EntityManager::tick() {
    // Placeholder for future entity updates
    // This could handle:
    // - Periodic position broadcasts
    // - Entity AI updates
    // - Entity despawning when players move out of range
}

bool EntityManager::is_in_range(const Player* p1, const Player* p2, f64 range) const {
    if (!p1 || !p2) {
        return false;
    }

    f64 dx = p1->get_x() - p2->get_x();
    f64 dy = p1->get_y() - p2->get_y();
    f64 dz = p1->get_z() - p2->get_z();

    f64 distance_squared = dx * dx + dy * dy + dz * dz;
    f64 range_squared = range * range;

    return distance_squared <= range_squared;
}

} // namespace mcserver
