#pragma once

#include "util/types.hpp"
#include "entity/player.hpp"
#include "entity/entity_id_manager.hpp"
#include <vector>
#include <unordered_map>
#include <memory>
#include <functional>

namespace mcserver {

class ClientSession;

// Callback for spawning an entity to a specific client
using SpawnPlayerCallback = std::function<void(ClientSession* viewer, const Player* player)>;
// Callback for despawning an entity from a specific client
using DespawnEntityCallback = std::function<void(ClientSession* viewer, i32 entity_id)>;
// Callback for when player health changes (entity_id, new_health, took_damage)
using HealthChangeCallback = std::function<void(i32 entity_id, i16 health, bool took_damage)>;
// Callback for when player dies (entity_id)
using PlayerDeathCallback = std::function<void(i32 entity_id)>;

// EntityManager tracks all entities in the world and manages entity visibility
class EntityManager {
public:
    EntityManager() = default;

    // Get entity ID manager
    EntityIdManager* get_id_manager() { return &id_manager_; }

    // Set callbacks
    void set_spawn_player_callback(SpawnPlayerCallback callback) {
        spawn_player_callback_ = std::move(callback);
    }

    void set_despawn_entity_callback(DespawnEntityCallback callback) {
        despawn_entity_callback_ = std::move(callback);
    }

    void set_health_change_callback(HealthChangeCallback callback) {
        health_change_callback_ = std::move(callback);
    }

    void set_death_callback(PlayerDeathCallback callback) {
        death_callback_ = std::move(callback);
    }

    // Player management
    void add_player(Player* player, ClientSession* session);
    void remove_player(i32 entity_id);
    Player* get_player(i32 entity_id);

    // Get all players except the specified one
    std::vector<Player*> get_other_players(i32 exclude_entity_id);

    // Get all players as a vector
    std::vector<Player*> get_all_players();

    // Get client session for a player
    ClientSession* get_player_session(i32 entity_id);

    // Entity visibility management
    void spawn_existing_entities_for(ClientSession* new_client);
    void spawn_entity_for_nearby_players(Player* player, ClientSession* exclude_session = nullptr);
    void despawn_entity_for_all(i32 entity_id);

    // Tick for entity updates
    void tick();

private:
    // Entity ID manager for allocation/deallocation
    EntityIdManager id_manager_;

    // Map of entity ID -> Player
    std::unordered_map<i32, Player*> players_;
    // Map of entity ID -> ClientSession (for sending packets)
    std::unordered_map<i32, ClientSession*> player_sessions_;

    SpawnPlayerCallback spawn_player_callback_;
    DespawnEntityCallback despawn_entity_callback_;
    HealthChangeCallback health_change_callback_;
    PlayerDeathCallback death_callback_;

    // Check if two entities are within visibility range
    bool is_in_range(const Player* p1, const Player* p2, f64 range = 128.0) const;
};

} // namespace mcserver
