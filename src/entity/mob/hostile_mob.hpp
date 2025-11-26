#pragma once

#include "entity/mob/mob.hpp"
#include "entity/mob/pathfinding.hpp"

namespace mcserver {

class Player;
class ChunkManager;

// Base class for hostile mobs
class HostileMob : public Mob {
public:
    HostileMob(i32 entity_id, MobType type);

    bool is_hostile() const override { return true; }
    f32 get_movement_speed() const override { return 0.25f; } // Slightly faster than passive

    void update_ai() override;

    // Set the player list for targeting
    void set_player_list(const std::vector<Player*>* players) { players_ = players; }

    // Set chunk manager for pathfinding
    void set_chunk_manager(ChunkManager* chunk_manager) { chunk_manager_ = chunk_manager; }

protected:
    // Find nearest player within range
    Player* find_nearest_player(f64 max_range);

    // Chase target player
    void chase_target(Player* target);

    // Attack target (to be implemented)
    virtual void attack_target(Player* target);

    const std::vector<Player*>* players_ = nullptr;
    Player* target_player_ = nullptr;
    i32 attack_cooldown_ = 0;

    // Pathfinding
    ChunkManager* chunk_manager_ = nullptr;
    PathFollower path_follower_;
    i32 pathfind_cooldown_ = 0;  // Ticks until next pathfinding attempt
};

// Zombie mob (ID 54)
class MobZombie : public HostileMob {
public:
    explicit MobZombie(i32 entity_id);
    void attack_target(Player* target) override;
};

// Skeleton mob (ID 51)
class MobSkeleton : public HostileMob {
public:
    explicit MobSkeleton(i32 entity_id);
    f32 get_movement_speed() const override { return 0.23f; } // Slightly slower
    void attack_target(Player* target) override;
};

// Creeper mob (ID 50)
class MobCreeper : public HostileMob {
public:
    explicit MobCreeper(i32 entity_id);
    void update_ai() override;
    void attack_target(Player* target) override;

private:
    i32 fuse_time_ = 0;
    bool is_ignited_ = false;
};

// Spider mob (ID 52)
class MobSpider : public HostileMob {
public:
    explicit MobSpider(i32 entity_id);
    f32 get_movement_speed() const override { return 0.3f; } // Fastest hostile mob
    void attack_target(Player* target) override;
};

} // namespace mcserver
