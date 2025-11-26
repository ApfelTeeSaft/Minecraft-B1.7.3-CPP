#pragma once

#include "entity/mob/mob.hpp"
#include "entity/mob/mob_type.hpp"
#include "entity/mob/mob_spawner.hpp"
#include "util/types.hpp"
#include <memory>
#include <unordered_map>
#include <vector>
#include <functional>

namespace mcserver {

class ClientSession;
class ChunkManager;
class Player;
class MobSpawner;

// Callback for broadcasting mob spawn packets
using MobSpawnCallback = std::function<void(const Mob* mob)>;

// Callback for broadcasting mob movement packets
using MobMovementCallback = std::function<void(i32 entity_id, f64 old_x, f64 old_y, f64 old_z,
                                                f64 new_x, f64 new_y, f64 new_z,
                                                f32 yaw, f32 pitch)>;

// Callback for broadcasting mob despawn packets
using MobDespawnCallback = std::function<void(i32 entity_id)>;

// Manages all mobs in the world
class MobManager {
public:
    explicit MobManager(ChunkManager* chunk_manager);

    // Spawn a mob at the given position
    Mob* spawn_mob(MobType type, f64 x, f64 y, f64 z);

    // Remove a mob by entity ID
    void remove_mob(i32 entity_id);

    // Update all mobs (called every server tick)
    void update_all();

    // Get a mob by entity ID
    Mob* get_mob(i32 entity_id);
    const Mob* get_mob(i32 entity_id) const;

    // Get all mobs
    const std::unordered_map<i32, std::unique_ptr<Mob>>& get_all_mobs() const {
        return mobs_;
    }

    // Spawn existing mobs for a new player
    void spawn_existing_mobs_for(ClientSession* session);

    // Set callback for mob spawn events
    void set_spawn_callback(MobSpawnCallback callback) {
        spawn_callback_ = callback;
    }

    // Set callback for mob movement events
    void set_movement_callback(MobMovementCallback callback) {
        movement_callback_ = callback;
    }

    // Set callback for mob despawn events
    void set_despawn_callback(MobDespawnCallback callback) {
        despawn_callback_ = callback;
    }

    // Simple mob spawning around spawn point (for testing)
    void spawn_test_mobs(f64 spawn_x, f64 spawn_z);

    // Spawn test hostile mobs for testing
    void spawn_test_hostile_mobs(f64 spawn_x, f64 spawn_z);

    // Set the player list for hostile mob targeting
    void set_player_list(const std::vector<Player*>* players) { players_ = players; }

    // Get natural spawner
    MobSpawner* get_spawner() { return spawner_.get(); }

private:
    ChunkManager* chunk_manager_;
    std::unordered_map<i32, std::unique_ptr<Mob>> mobs_;
    std::unique_ptr<MobSpawner> spawner_;
    MobSpawnCallback spawn_callback_;
    MobMovementCallback movement_callback_;
    MobDespawnCallback despawn_callback_;
    const std::vector<Player*>* players_ = nullptr;
    i32 next_entity_id_ = 1000;  // Start at 1000 to avoid conflict with players

    // Get next available entity ID
    i32 get_next_entity_id() { return next_entity_id_++; }

    // Broadcast mob movement to all clients
    void broadcast_mob_movement(i32 entity_id, f64 old_x, f64 old_y, f64 old_z,
                               f64 new_x, f64 new_y, f64 new_z, f32 yaw, f32 pitch);

    // Get ground level at given position (finds top solid block)
    f64 get_ground_level(f64 x, f64 z);
};

} // namespace mcserver
