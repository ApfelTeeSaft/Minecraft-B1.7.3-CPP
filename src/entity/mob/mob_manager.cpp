#include "entity/mob/mob_manager.hpp"
#include "entity/mob/passive_mob.hpp"
#include "entity/mob/hostile_mob.hpp"
#include "entity/mob/mob_spawner.hpp"
#include "net/session/client_session.hpp"
#include "net/protocol/packets/mob_spawn.hpp"
#include "world/chunk/chunk.hpp"
#include "world/chunk/chunk_manager.hpp"
#include "util/log/logger.hpp"
#include <random>
#include <cmath>

namespace mcserver {

MobManager::MobManager(ChunkManager* chunk_manager)
    : chunk_manager_(chunk_manager)
    , spawner_(std::make_unique<MobSpawner>(this, chunk_manager)) {
}

Mob* MobManager::spawn_mob(MobType type, f64 x, f64 y, f64 z) {
    i32 entity_id = get_next_entity_id();

    // Create the mob based on type
    std::unique_ptr<Mob> mob;
    switch (type) {
        // Passive mobs
        case MobType::Pig:
            mob = std::make_unique<MobPig>(entity_id);
            break;
        case MobType::Sheep:
            mob = std::make_unique<MobSheep>(entity_id);
            break;
        case MobType::Cow:
            mob = std::make_unique<MobCow>(entity_id);
            break;
        case MobType::Chicken:
            mob = std::make_unique<MobChicken>(entity_id);
            break;

        // Hostile mobs
        case MobType::Zombie:
            mob = std::make_unique<MobZombie>(entity_id);
            break;
        case MobType::Skeleton:
            mob = std::make_unique<MobSkeleton>(entity_id);
            break;
        case MobType::Creeper:
            mob = std::make_unique<MobCreeper>(entity_id);
            break;
        case MobType::Spider:
            mob = std::make_unique<MobSpider>(entity_id);
            break;

        default:
            LOG_WARNING_CAT("Unsupported mob type: " + std::to_string(static_cast<i8>(type)),
                           LogCategory::Entity);
            return nullptr;
    }

    mob->set_position(x, y, z);

    // Set up movement callback for this mob
    mob->set_move_callback([this](i32 entity_id, f64 old_x, f64 old_y, f64 old_z,
                                  f64 new_x, f64 new_y, f64 new_z,
                                  f32 yaw, f32 pitch) {
        this->broadcast_mob_movement(entity_id, old_x, old_y, old_z, new_x, new_y, new_z, yaw, pitch);
    });

    // If this is a hostile mob, give it access to the player list and chunk manager
    if (mob->is_hostile()) {
        auto* hostile = dynamic_cast<HostileMob*>(mob.get());
        if (hostile) {
            if (players_) {
                hostile->set_player_list(players_);
            }
            if (chunk_manager_) {
                hostile->set_chunk_manager(chunk_manager_);
            }
        }
    }

    // Broadcast spawn to all clients
    if (spawn_callback_) {
        spawn_callback_(mob.get());
    }

    LOG_INFO_CAT("Spawned " + mob->get_name() + " at (" +
                std::to_string(x) + ", " + std::to_string(y) + ", " + std::to_string(z) + ")",
                LogCategory::Entity);

    Mob* mob_ptr = mob.get();
    mobs_[entity_id] = std::move(mob);
    return mob_ptr;
}

void MobManager::remove_mob(i32 entity_id) {
    auto it = mobs_.find(entity_id);
    if (it != mobs_.end()) {
        LOG_INFO_CAT("Removed mob " + it->second->get_name() + " (ID: " +
                    std::to_string(entity_id) + ")", LogCategory::Entity);
        mobs_.erase(it);
    }
}

void MobManager::update_all() {
    // Collect mobs ready to despawn (can't modify map while iterating)
    std::vector<i32> mobs_to_remove;

    for (auto& [id, mob] : mobs_) {
        // Always update mobs (even dead ones, for death timer)
        mob->update();

        // Check if dead mob should despawn after animation
        if (mob->should_despawn()) {
            mobs_to_remove.push_back(id);
        }
    }

    // Remove mobs that finished death animation
    for (i32 entity_id : mobs_to_remove) {
        auto it = mobs_.find(entity_id);
        if (it != mobs_.end()) {
            LOG_INFO_CAT("Removed mob: " + it->second->get_name() + " (ID: " +
                        std::to_string(entity_id) + ")", LogCategory::Entity);

            // Broadcast despawn to all clients
            if (despawn_callback_) {
                despawn_callback_(entity_id);
            }

            // Remove from manager
            mobs_.erase(it);
        }
    }
}

Mob* MobManager::get_mob(i32 entity_id) {
    auto it = mobs_.find(entity_id);
    return (it != mobs_.end()) ? it->second.get() : nullptr;
}

const Mob* MobManager::get_mob(i32 entity_id) const {
    auto it = mobs_.find(entity_id);
    return (it != mobs_.end()) ? it->second.get() : nullptr;
}

void MobManager::spawn_existing_mobs_for(ClientSession* session) {
    if (!session) {
        return;
    }

    for (const auto& [id, mob] : mobs_) {
        PacketMobSpawn spawn_packet(mob.get());
        session->send_packet(spawn_packet);
    }

    LOG_DEBUG_CAT("Sent " + std::to_string(mobs_.size()) + " existing mobs to " +
                 session->get_username(), LogCategory::Entity);
}

void MobManager::spawn_test_mobs(f64 spawn_x, f64 spawn_z) {
    // Spawn a few test mobs in a circle around the spawn point
    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_real_distribution<> angle_dist(0.0, 6.28318530718);  // 0 to 2*PI
    std::uniform_real_distribution<> radius_dist(10.0, 30.0);

    // Spawn 2 of each passive mob type
    MobType types[] = {MobType::Pig, MobType::Sheep, MobType::Cow, MobType::Chicken};

    for (MobType type : types) {
        for (int i = 0; i < 2; ++i) {
            f64 angle = angle_dist(gen);
            f64 radius = radius_dist(gen);

            f64 x = spawn_x + radius * std::cos(angle);
            f64 z = spawn_z + radius * std::sin(angle);
            f64 y = get_ground_level(x, z);  // Get actual ground level from terrain

            spawn_mob(type, x, y, z);
        }
    }

    LOG_INFO_CAT("Spawned " + std::to_string(mobs_.size()) + " passive test mobs", LogCategory::Entity);
}

void MobManager::spawn_test_hostile_mobs(f64 spawn_x, f64 spawn_z) {
    // Spawn hostile mobs in a circle around spawn
    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_real_distribution<> angle_dist(0.0, 6.28318530718);  // 0 to 2*PI
    std::uniform_real_distribution<> radius_dist(15.0, 40.0);  // Further away than passive mobs

    // Spawn 2 of each hostile mob type
    MobType types[] = {MobType::Zombie, MobType::Skeleton, MobType::Creeper, MobType::Spider};

    for (MobType type : types) {
        for (int i = 0; i < 2; ++i) {
            f64 angle = angle_dist(gen);
            f64 radius = radius_dist(gen);

            f64 x = spawn_x + radius * std::cos(angle);
            f64 z = spawn_z + radius * std::sin(angle);
            f64 y = get_ground_level(x, z);  // Get actual ground level from terrain

            spawn_mob(type, x, y, z);
        }
    }

    LOG_INFO_CAT("Spawned " + std::to_string(8) + " hostile test mobs", LogCategory::Entity);
}

void MobManager::broadcast_mob_movement(i32 entity_id, f64 old_x, f64 old_y, f64 old_z,
                                       f64 new_x, f64 new_y, f64 new_z, f32 yaw, f32 pitch) {
    if (movement_callback_) {
        movement_callback_(entity_id, old_x, old_y, old_z, new_x, new_y, new_z, yaw, pitch);
    }
}

f64 MobManager::get_ground_level(f64 x, f64 z) {
    if (!chunk_manager_) {
        return 64.0;  // Fallback if no chunk manager
    }

    // Convert world coordinates to chunk coordinates
    i32 world_x = static_cast<i32>(std::floor(x));
    i32 world_z = static_cast<i32>(std::floor(z));
    i32 chunk_x = world_x >> 4;  // Divide by 16
    i32 chunk_z = world_z >> 4;
    i32 local_x = world_x & 0xF; // Modulo 16
    i32 local_z = world_z & 0xF;

    // Get chunk
    Chunk* chunk = chunk_manager_->get_chunk(chunk_x, chunk_z);
    if (!chunk) {
        return 64.0;  // Fallback if chunk not loaded
    }

    // Find top solid block (start from top, search downward)
    for (i32 y = 127; y > 0; y--) {
        u8 block = chunk->get_block(local_x, y, local_z);
        if (block != 0) {  // Found first non-air block
            return static_cast<f64>(y) + 1.0;  // Spawn 1 block above
        }
    }

    return 64.0;  // Fallback if no solid block found
}

} // namespace mcserver
