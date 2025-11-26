#include "entity/mob/hostile_mob.hpp"
#include "entity/player.hpp"
#include "world/chunk/chunk_manager.hpp"
#include "util/log/logger.hpp"
#include <cmath>
#include <limits>

namespace mcserver {

HostileMob::HostileMob(i32 entity_id, MobType type)
    : Mob(entity_id, type) {
}

void HostileMob::update_ai() {
    // Reduce cooldowns
    if (attack_cooldown_ > 0) {
        attack_cooldown_--;
    }
    if (pathfind_cooldown_ > 0) {
        pathfind_cooldown_--;
    }

    // Try to find a target player
    Player* nearest = find_nearest_player(16.0);  // 16 block detection range

    if (nearest) {
        target_player_ = nearest;
        ai_state_ = MobAIState::Chasing;
        chase_target(nearest);

        // Check if close enough to attack (< 2 blocks)
        f64 dx = nearest->get_x() - x_;
        f64 dy = nearest->get_y() - y_;
        f64 dz = nearest->get_z() - z_;
        f64 dist_sq = dx * dx + dy * dy + dz * dz;

        if (dist_sq < 4.0 && attack_cooldown_ <= 0) {  // Within 2 blocks
            ai_state_ = MobAIState::Attacking;
            attack_target(nearest);
            attack_cooldown_ = 20;  // 1 second cooldown (20 ticks)
        }
    } else {
        // No target, wander randomly
        target_player_ = nullptr;
        path_follower_.clear_path();
        if (ai_state_ != MobAIState::Idle && ai_state_ != MobAIState::Wandering) {
            ai_state_ = MobAIState::Idle;
        }
        wander_randomly();
    }
}

Player* HostileMob::find_nearest_player(f64 max_range) {
    if (!players_) {
        return nullptr;
    }

    Player* nearest = nullptr;
    f64 nearest_dist_sq = max_range * max_range;

    for (Player* player : *players_) {
        if (!player) continue;

        f64 dx = player->get_x() - x_;
        f64 dy = player->get_y() - y_;
        f64 dz = player->get_z() - z_;
        f64 dist_sq = dx * dx + dy * dy + dz * dz;

        if (dist_sq < nearest_dist_sq) {
            nearest_dist_sq = dist_sq;
            nearest = player;
        }
    }

    return nearest;
}

void HostileMob::chase_target(Player* target) {
    if (!target) {
        return;
    }

    f64 target_x, target_y, target_z;

    // Try to use pathfinding if available and we don't have a path
    if (chunk_manager_ && !path_follower_.has_path() && pathfind_cooldown_ <= 0) {
        // Attempt pathfinding every 40 ticks (2 seconds) to avoid overhead
        pathfind_cooldown_ = 40;

        Pathfinder pathfinder(chunk_manager_);
        PathNode start{static_cast<i32>(std::floor(x_)),
                      static_cast<i32>(std::floor(y_)),
                      static_cast<i32>(std::floor(z_))};
        PathNode goal{static_cast<i32>(std::floor(target->get_x())),
                     static_cast<i32>(std::floor(target->get_y())),
                     static_cast<i32>(std::floor(target->get_z()))};

        auto result = pathfinder.find_path(start, goal, 32.0, true, false);
        if (result.success && !result.path.empty()) {
            path_follower_.set_path(result.path);
        }
    }

    // Follow path if we have one
    if (path_follower_.get_next_waypoint(x_, y_, z_, target_x, target_y, target_z)) {
        // Move towards waypoint
        f64 dx = target_x - x_;
        f64 dz = target_z - z_;

        f64 angle = std::atan2(-dx, dz);
        yaw_ = static_cast<f32>(angle * 180.0 / 3.14159265359);

        // Normalize yaw
        while (yaw_ < 0.0f) yaw_ += 360.0f;
        while (yaw_ >= 360.0f) yaw_ -= 360.0f;

        movement_.is_moving = true;
        movement_.target_yaw = yaw_;
    } else {
        // No path, move directly towards target (simple chase)
        f64 dx = target->get_x() - x_;
        f64 dz = target->get_z() - z_;

        f64 angle = std::atan2(-dx, dz);
        yaw_ = static_cast<f32>(angle * 180.0 / 3.14159265359);

        // Normalize yaw
        while (yaw_ < 0.0f) yaw_ += 360.0f;
        while (yaw_ >= 360.0f) yaw_ -= 360.0f;

        movement_.is_moving = true;
        movement_.target_yaw = yaw_;
    }
}

void HostileMob::attack_target(Player* target) {
    // Base implementation - will be overridden by specific mobs
    if (target) {
        LOG_DEBUG_CAT(get_name() + " attacks " + target->get_username(), LogCategory::Entity);
    }
}

// Zombie implementation
MobZombie::MobZombie(i32 entity_id)
    : HostileMob(entity_id, MobType::Zombie) {
    health_ = 20;
    max_health_ = 20;
}

void MobZombie::attack_target(Player* target) {
    if (!target || target->is_dead()) return;

    // Zombies deal 2-3 hearts of damage (4-6 HP) on normal difficulty
    // In Beta 1.7.3, zombies deal damage when very close
    i16 damage = 5;  // 2.5 hearts
    target->take_damage(damage);

    LOG_DEBUG_CAT("Zombie attacks " + target->get_username() + " for " +
                 std::to_string(damage) + " damage (HP: " + std::to_string(target->get_health()) + "/20)",
                 LogCategory::Entity);
}

// Skeleton implementation
MobSkeleton::MobSkeleton(i32 entity_id)
    : HostileMob(entity_id, MobType::Skeleton) {
    health_ = 20;
    max_health_ = 20;
}

void MobSkeleton::attack_target(Player* target) {
    if (!target || target->is_dead()) return;

    // Skeletons shoot arrows from a distance
    // In Beta 1.7.3, skeletons have a range of ~15 blocks
    f64 dx = target->get_x() - x_;
    f64 dz = target->get_z() - z_;
    f64 dist = std::sqrt(dx * dx + dz * dz);

    if (dist > 4.0 && dist < 15.0) {  // Shoot from 4-15 blocks away
        // For now, apply instant damage (arrow damage)
        // TODO: Spawn arrow entity when projectile system is implemented
        i16 damage = 4;  // 2 hearts
        target->take_damage(damage);

        LOG_DEBUG_CAT("Skeleton shoots arrow at " + target->get_username() + " for " +
                     std::to_string(damage) + " damage (HP: " + std::to_string(target->get_health()) + "/20)",
                     LogCategory::Entity);
    }
}

// Creeper implementation
MobCreeper::MobCreeper(i32 entity_id)
    : HostileMob(entity_id, MobType::Creeper) {
    health_ = 20;
    max_health_ = 20;
}

void MobCreeper::update_ai() {
    // Creepers have special behavior - they explode when close to players
    Player* nearest = find_nearest_player(16.0);

    if (nearest) {
        target_player_ = nearest;

        f64 dx = nearest->get_x() - x_;
        f64 dy = nearest->get_y() - y_;
        f64 dz = nearest->get_z() - z_;
        f64 dist_sq = dx * dx + dy * dy + dz * dz;

        if (dist_sq < 9.0) {  // Within 3 blocks - start fusing
            if (!is_ignited_) {
                is_ignited_ = true;
                fuse_time_ = 30;  // 1.5 seconds fuse (30 ticks)
                LOG_DEBUG_CAT("Creeper ignited near " + nearest->get_username(), LogCategory::Entity);
            }

            // Stand still while fusing
            movement_.is_moving = false;
            ai_state_ = MobAIState::Attacking;

            fuse_time_--;
            if (fuse_time_ <= 0) {
                // Explode! Deal damage to nearby player
                if (nearest && !nearest->is_dead()) {
                    i16 damage = 17;  // 8.5 hearts - very high damage
                    nearest->take_damage(damage);

                    LOG_INFO_CAT("Creeper exploded at (" + std::to_string(x_) + ", " +
                                std::to_string(y_) + ", " + std::to_string(z_) + ") dealing " +
                                std::to_string(damage) + " damage to " + nearest->get_username(),
                                LogCategory::Entity);
                } else {
                    LOG_INFO_CAT("Creeper exploded at (" + std::to_string(x_) + ", " +
                                std::to_string(y_) + ", " + std::to_string(z_) + ")",
                                LogCategory::Entity);
                }
                // TODO: Create explosion block destruction
                health_ = 0;  // Creeper dies after exploding
            }
        } else if (dist_sq < 256.0) {  // Within 16 blocks - chase
            if (is_ignited_ && dist_sq > 16.0) {
                // Player moved away, defuse
                is_ignited_ = false;
                fuse_time_ = 0;
            }
            ai_state_ = MobAIState::Chasing;
            chase_target(nearest);
        } else {
            // Too far, wander
            is_ignited_ = false;
            fuse_time_ = 0;
            wander_randomly();
        }
    } else {
        // No target
        is_ignited_ = false;
        fuse_time_ = 0;
        target_player_ = nullptr;
        wander_randomly();
    }
}

void MobCreeper::attack_target(Player* target) {
    (void)target;  // Unused - creepers explode instead of traditional attack
    // Creepers don't have a traditional attack - they explode
    // The explosion logic is handled in update_ai()
}

// Spider implementation
MobSpider::MobSpider(i32 entity_id)
    : HostileMob(entity_id, MobType::Spider) {
    health_ = 16;
    max_health_ = 16;
}

void MobSpider::attack_target(Player* target) {
    if (!target || target->is_dead()) return;

    // Spiders are fast and deal poison damage in later versions
    // In Beta 1.7.3, they just deal regular melee damage
    i16 damage = 3;  // 1.5 hearts
    target->take_damage(damage);

    LOG_DEBUG_CAT("Spider attacks " + target->get_username() + " for " +
                 std::to_string(damage) + " damage (HP: " + std::to_string(target->get_health()) + "/20)",
                 LogCategory::Entity);
}

} // namespace mcserver
