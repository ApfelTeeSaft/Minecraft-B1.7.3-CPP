#pragma once

#include "mob_type.hpp"
#include "util/types.hpp"
#include <vector>
#include <random>

namespace mcserver {

class MobManager;
class ChunkManager;
class Player;

// Spawn group: defines what mobs can spawn together
struct SpawnGroup {
    MobType mob_type;
    i32 min_group_size;
    i32 max_group_size;
    i32 weight;  // Spawn weight (higher = more common)
};

// Natural mob spawning system for Beta 1.7.3
class MobSpawner {
public:
    explicit MobSpawner(MobManager* mob_manager, ChunkManager* chunk_manager);

    // Update spawning (called every tick)
    void tick(const std::vector<Player*>& players);

    // Set spawn limits
    void set_spawn_limit(i32 limit) { spawn_limit_ = limit; }
    i32 get_spawn_limit() const { return spawn_limit_; }

    // Enable/disable natural spawning
    void set_enabled(bool enabled) { enabled_ = enabled; }
    bool is_enabled() const { return enabled_; }

private:
    MobManager* mob_manager_;
    ChunkManager* chunk_manager_;
    std::mt19937 random_gen_;

    bool enabled_ = true;
    i32 spawn_limit_ = 70;  // Beta 1.7.3 default mob cap
    i32 spawn_cycle_counter_ = 0;
    i32 spawn_cycle_interval_ = 20;  // Spawn every 20 ticks (1 second)

    // Spawn attempt constants
    static constexpr i32 SPAWN_ATTEMPTS_PER_CYCLE = 3;
    static constexpr i32 MIN_SPAWN_DISTANCE = 24;  // Min blocks from player
    static constexpr i32 MAX_SPAWN_DISTANCE = 128; // Max blocks from player (chunk load distance)
    static constexpr i32 MAX_SPAWN_HEIGHT = 120;   // Don't spawn near world height
    static constexpr i32 MIN_SPAWN_HEIGHT = 1;     // Don't spawn at bedrock

    // Spawn groups for different mob types
    static const std::vector<SpawnGroup> hostile_spawns_;
    static const std::vector<SpawnGroup> passive_spawns_;

    // Attempt to spawn mobs near a player
    void attempt_spawn_near_player(const Player* player);

    // Try to spawn a mob at a specific location
    bool try_spawn_mob(MobType type, f64 x, f64 y, f64 z);

    // Check if location is valid for spawning
    bool is_valid_spawn_location(MobType type, i32 x, i32 y, i32 z);

    // Check light level at location
    u8 get_light_level(i32 x, i32 y, i32 z);

    // Check if block is solid
    bool is_solid_block(u8 block_id);

    // Check if block is liquid
    bool is_liquid_block(u8 block_id);

    // Get random spawn group based on weights
    const SpawnGroup* get_random_spawn_group(const std::vector<SpawnGroup>& groups);

    // Count current mobs
    i32 count_mobs();
};

} // namespace mcserver
