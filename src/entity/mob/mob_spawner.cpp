#include "mob_spawner.hpp"
#include "mob_manager.hpp"
#include "entity/player.hpp"
#include "world/chunk/chunk_manager.hpp"
#include "world/chunk/chunk.hpp"
#include "util/log/logger.hpp"
#include <algorithm>

namespace mcserver {

// Hostile mob spawn groups (spawn in darkness, light level <= 7)
const std::vector<SpawnGroup> MobSpawner::hostile_spawns_ = {
    {MobType::Zombie, 4, 4, 100},    // Common, groups of 4
    {MobType::Skeleton, 4, 4, 100},  // Common, groups of 4
    {MobType::Spider, 4, 4, 100},    // Common, groups of 4
    {MobType::Creeper, 4, 4, 100},   // Common, groups of 4
};

// Passive mob spawn groups (spawn in light, light level >= 9)
const std::vector<SpawnGroup> MobSpawner::passive_spawns_ = {
    {MobType::Pig, 4, 4, 100},       // Common, groups of 4
    {MobType::Sheep, 4, 4, 100},     // Common, groups of 4
    {MobType::Cow, 4, 4, 100},       // Common, groups of 4
    {MobType::Chicken, 4, 4, 100},   // Common, groups of 4
};

MobSpawner::MobSpawner(MobManager* mob_manager, ChunkManager* chunk_manager)
    : mob_manager_(mob_manager)
    , chunk_manager_(chunk_manager) {
    std::random_device rd;
    random_gen_.seed(rd());
}

void MobSpawner::tick(const std::vector<Player*>& players) {
    if (!enabled_ || players.empty()) {
        return;
    }

    // Spawn cycle: attempt spawns every spawn_cycle_interval_ ticks
    spawn_cycle_counter_++;
    if (spawn_cycle_counter_ < spawn_cycle_interval_) {
        return;
    }
    spawn_cycle_counter_ = 0;

    // Check mob cap
    i32 current_mob_count = count_mobs();
    if (current_mob_count >= spawn_limit_) {
        return;
    }

    // Attempt spawns for each player
    for (const Player* player : players) {
        if (!player) continue;

        // Make multiple spawn attempts per cycle
        for (i32 i = 0; i < SPAWN_ATTEMPTS_PER_CYCLE; ++i) {
            attempt_spawn_near_player(player);
        }
    }
}

void MobSpawner::attempt_spawn_near_player(const Player* player) {
    if (!player) return;

    // Get player position
    f64 player_x = player->get_x();
    f64 player_z = player->get_z();

    // Random angle and distance from player
    std::uniform_real_distribution<f64> angle_dist(0.0, 6.28318530718); // 0 to 2*PI
    std::uniform_real_distribution<f64> distance_dist(MIN_SPAWN_DISTANCE, MAX_SPAWN_DISTANCE);
    std::uniform_int_distribution<i32> height_dist(MIN_SPAWN_HEIGHT, MAX_SPAWN_HEIGHT);

    f64 angle = angle_dist(random_gen_);
    f64 distance = distance_dist(random_gen_);

    // Calculate spawn position
    f64 spawn_x = player_x + distance * std::cos(angle);
    f64 spawn_z = player_z + distance * std::sin(angle);

    // Try multiple heights to find valid spawn
    for (i32 attempt = 0; attempt < 5; ++attempt) {
        i32 spawn_y = height_dist(random_gen_);

        // Decide hostile or passive based on light level
        u8 light = get_light_level(static_cast<i32>(spawn_x), spawn_y, static_cast<i32>(spawn_z));

        const SpawnGroup* group = nullptr;
        if (light <= 7) {
            // Dark area - spawn hostile mobs
            group = get_random_spawn_group(hostile_spawns_);
        } else if (light >= 9) {
            // Bright area - spawn passive mobs
            group = get_random_spawn_group(passive_spawns_);
        } else {
            // Twilight zone - 50/50 chance
            std::uniform_int_distribution<i32> coin_flip(0, 1);
            if (coin_flip(random_gen_) == 0) {
                group = get_random_spawn_group(hostile_spawns_);
            } else {
                group = get_random_spawn_group(passive_spawns_);
            }
        }

        if (!group) continue;

        // Try to spawn a group
        std::uniform_int_distribution<i32> group_size_dist(group->min_group_size, group->max_group_size);
        i32 group_size = group_size_dist(random_gen_);

        i32 spawned = 0;
        for (i32 i = 0; i < group_size; ++i) {
            // Slight offset for each mob in group
            std::uniform_real_distribution<f64> offset_dist(-2.0, 2.0);
            f64 offset_x = offset_dist(random_gen_);
            f64 offset_z = offset_dist(random_gen_);

            f64 mob_x = spawn_x + offset_x;
            f64 mob_z = spawn_z + offset_z;

            if (try_spawn_mob(group->mob_type, mob_x, spawn_y, mob_z)) {
                spawned++;
            }
        }

        if (spawned > 0) {
            // Successfully spawned some mobs, done for this attempt
            return;
        }
    }
}

bool MobSpawner::try_spawn_mob(MobType type, f64 x, f64 y, f64 z) {
    i32 ix = static_cast<i32>(std::floor(x));
    i32 iy = static_cast<i32>(std::floor(y));
    i32 iz = static_cast<i32>(std::floor(z));

    // Check if location is valid
    if (!is_valid_spawn_location(type, ix, iy, iz)) {
        return false;
    }

    // Spawn the mob
    mob_manager_->spawn_mob(type, x, y, z);
    return true;
}

bool MobSpawner::is_valid_spawn_location(MobType type, i32 x, i32 y, i32 z) {
    // Get the chunk
    i32 chunk_x = x >> 4;
    i32 chunk_z = z >> 4;
    Chunk* chunk = chunk_manager_->get_chunk_if_loaded(chunk_x, chunk_z);
    if (!chunk) {
        return false;  // Chunk not loaded
    }

    // Convert to local chunk coordinates
    i32 local_x = x & 0xF;
    i32 local_z = z & 0xF;

    // Check bounds
    if (y < 0 || y >= CHUNK_SIZE_Y - 2) {
        return false;
    }

    // Get blocks at spawn location
    u8 block_below = chunk->get_block(local_x, y - 1, local_z);
    u8 block_at = chunk->get_block(local_x, y, local_z);
    u8 block_above = chunk->get_block(local_x, y + 1, local_z);

    // Need solid block below
    if (!is_solid_block(block_below)) {
        return false;
    }

    // Need air (or non-solid) at spawn position and above
    if (block_at != static_cast<u8>(BlockId::Air) || block_above != static_cast<u8>(BlockId::Air)) {
        return false;
    }

    // Don't spawn in liquids
    if (is_liquid_block(block_below)) {
        return false;
    }

    // Check light level
    u8 light = get_light_level(x, y, z);

    // Hostile mobs need darkness (light <= 7)
    bool is_hostile = (type == MobType::Zombie || type == MobType::Skeleton ||
                      type == MobType::Creeper || type == MobType::Spider);

    if (is_hostile && light > 7) {
        return false;
    }

    // Passive mobs need light (light >= 9)
    if (!is_hostile && light < 9) {
        return false;
    }

    return true;
}

u8 MobSpawner::get_light_level(i32 x, i32 y, i32 z) {
    // Get the chunk
    i32 chunk_x = x >> 4;
    i32 chunk_z = z >> 4;
    Chunk* chunk = chunk_manager_->get_chunk_if_loaded(chunk_x, chunk_z);
    if (!chunk) {
        return 15;  // Assume full light if chunk not loaded
    }

    // Convert to local chunk coordinates
    i32 local_x = x & 0xF;
    i32 local_z = z & 0xF;

    // Check bounds
    if (y < 0 || y >= CHUNK_SIZE_Y) {
        return 15;
    }

    // Get max of block light and sky light
    u8 block_light = chunk->get_block_light(local_x, y, local_z);
    u8 sky_light = chunk->get_sky_light(local_x, y, local_z);
    return std::max(block_light, sky_light);
}

bool MobSpawner::is_solid_block(u8 block_id) {
    // Air and transparent blocks are not solid
    return block_id != static_cast<u8>(BlockId::Air) &&
           block_id != static_cast<u8>(BlockId::Glass) &&
           block_id != static_cast<u8>(BlockId::Sapling) &&
           !is_liquid_block(block_id);
}

bool MobSpawner::is_liquid_block(u8 block_id) {
    return block_id == static_cast<u8>(BlockId::WaterFlowing) ||
           block_id == static_cast<u8>(BlockId::WaterStill) ||
           block_id == static_cast<u8>(BlockId::LavaFlowing) ||
           block_id == static_cast<u8>(BlockId::LavaStill);
}

const SpawnGroup* MobSpawner::get_random_spawn_group(const std::vector<SpawnGroup>& groups) {
    if (groups.empty()) {
        return nullptr;
    }

    // Calculate total weight
    i32 total_weight = 0;
    for (const auto& group : groups) {
        total_weight += group.weight;
    }

    // Random weighted selection
    std::uniform_int_distribution<i32> weight_dist(0, total_weight - 1);
    i32 random_weight = weight_dist(random_gen_);

    i32 cumulative_weight = 0;
    for (const auto& group : groups) {
        cumulative_weight += group.weight;
        if (random_weight < cumulative_weight) {
            return &group;
        }
    }

    return &groups[0];  // Fallback
}

i32 MobSpawner::count_mobs() {
    return static_cast<i32>(mob_manager_->get_all_mobs().size());
}

} // namespace mcserver
