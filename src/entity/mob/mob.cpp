#include "entity/mob/mob.hpp"
#include <algorithm>
#include <cmath>
#include <random>

namespace mcserver {

Mob::Mob(i32 entity_id, MobType type)
    : entity_id_(entity_id), mob_type_(type) {
    movement_.move_speed = get_movement_speed();

    // Initialize required metadata for Beta 1.7.3
    // Index 0: Entity flags (fire, crouched, riding, sprinting, eating)
    metadata_.set_byte(0, 0x00);  // No flags set by default
}

void Mob::set_position(f64 x, f64 y, f64 z) {
    prev_x_ = x_;
    prev_y_ = y_;
    prev_z_ = z_;
    x_ = x;
    y_ = y;
    z_ = z;
}

void Mob::set_rotation(f32 yaw, f32 pitch) {
    yaw_ = yaw;
    pitch_ = pitch;
}

void Mob::set_health(i16 health) {
    health_ = std::clamp(health, static_cast<i16>(0), max_health_);
}

void Mob::update() {
    age_++;

    // Handle death timer
    if (is_dead()) {
        death_timer_--;
        return;  // Dead mobs don't move or update AI
    }

    // Decrement panic timer
    if (panic_timer_ > 0) {
        panic_timer_--;
        if (panic_timer_ == 0) {
            ai_state_ = MobAIState::Idle;
        }
    }

    // Store previous position for movement detection
    prev_x_ = x_;
    prev_y_ = y_;
    prev_z_ = z_;

    // Update AI behavior
    update_ai();

    // Apply movement and physics
    apply_movement();

    // Check if mob moved significantly (> 0.01 blocks)
    f64 dx = x_ - prev_x_;
    f64 dy = y_ - prev_y_;
    f64 dz = z_ - prev_z_;
    f64 dist_sq = dx * dx + dy * dy + dz * dz;

    // Broadcast movement if the mob moved or rotated
    if (dist_sq > 0.0001 && move_callback_) {
        move_callback_(entity_id_, prev_x_, prev_y_, prev_z_, x_, y_, z_, yaw_, pitch_);
    }
}

std::string Mob::get_name() const {
    switch (mob_type_) {
        case MobType::Creeper: return "Creeper";
        case MobType::Skeleton: return "Skeleton";
        case MobType::Spider: return "Spider";
        case MobType::Giant: return "Giant";
        case MobType::Zombie: return "Zombie";
        case MobType::Slime: return "Slime";
        case MobType::Ghast: return "Ghast";
        case MobType::PigZombie: return "PigZombie";
        case MobType::Pig: return "Pig";
        case MobType::Sheep: return "Sheep";
        case MobType::Cow: return "Cow";
        case MobType::Chicken: return "Chicken";
        case MobType::Squid: return "Squid";
        case MobType::Wolf: return "Wolf";
        default: return "Unknown";
    }
}

void Mob::update_ai() {
    // Priority: Fleeing if in panic mode
    if (panic_timer_ > 0 && !is_hostile()) {
        ai_state_ = MobAIState::Fleeing;
        movement_.is_moving = true;

        // Calculate direction away from attacker
        f64 dx = x_ - flee_from_x_;
        f64 dz = z_ - flee_from_z_;
        f64 dist = std::sqrt(dx * dx + dz * dz);

        if (dist > 0.01) {
            // Face away from attacker
            f32 flee_yaw = static_cast<f32>(std::atan2(-dx, dz) * 180.0 / 3.14159265359);
            yaw_ = flee_yaw;
            movement_.target_yaw = flee_yaw;
        }

        // Run faster when panicking
        movement_.move_speed = get_movement_speed() * 1.5f;
        return;
    }

    // Reset movement speed to normal
    movement_.move_speed = get_movement_speed();

    // Base AI: Simple wandering for passive mobs
    if (!is_hostile()) {
        wander_randomly();
    }
}

void Mob::wander_randomly() {
    static std::random_device rd;
    static std::mt19937 gen(rd());
    static std::uniform_real_distribution<> change_dir_dist(0.0, 1.0);
    static std::uniform_real_distribution<> angle_dist(0.0, 6.28318530718);  // 0 to 2*PI

    // Random chance to change state
    if (change_dir_dist(gen) < 0.01) {  // 1% chance per tick to change state
        if (ai_state_ == MobAIState::Idle) {
            ai_state_ = MobAIState::Wandering;
            movement_.wander_ticks = 20 + static_cast<i32>(change_dir_dist(gen) * 60);  // 1-4 seconds
            movement_.target_yaw = static_cast<f32>(angle_dist(gen) * 180.0 / 3.14159265359);  // Random direction
        } else {
            ai_state_ = MobAIState::Idle;
            movement_.idle_ticks = 20 + static_cast<i32>(change_dir_dist(gen) * 80);  // 1-5 seconds
            movement_.is_moving = false;
        }
    }

    // Update state-specific behavior
    if (ai_state_ == MobAIState::Wandering) {
        movement_.is_moving = true;
        yaw_ = movement_.target_yaw;

        movement_.wander_ticks--;
        if (movement_.wander_ticks <= 0) {
            ai_state_ = MobAIState::Idle;
            movement_.is_moving = false;
        }
    } else if (ai_state_ == MobAIState::Idle) {
        movement_.is_moving = false;
        movement_.idle_ticks--;
    }
}

void Mob::apply_movement() {
    // Apply knockback/external velocity first
    x_ += movement_.velocity_x;
    y_ += movement_.velocity_y;
    z_ += movement_.velocity_z;

    // Apply friction to knockback velocity
    movement_.velocity_x *= 0.6;
    movement_.velocity_y *= 0.98;
    movement_.velocity_z *= 0.6;

    // Stop very small velocities
    if (std::abs(movement_.velocity_x) < 0.001) movement_.velocity_x = 0.0;
    if (std::abs(movement_.velocity_y) < 0.001) movement_.velocity_y = 0.0;
    if (std::abs(movement_.velocity_z) < 0.001) movement_.velocity_z = 0.0;

    // Apply movement velocity if moving
    if (movement_.is_moving) {
        // Convert yaw to radians
        f64 yaw_rad = yaw_ * 3.14159265359 / 180.0;

        // Calculate velocity based on yaw and speed
        // Minecraft uses Z-forward coordinate system
        f64 speed = movement_.move_speed / 20.0;  // Convert blocks/second to blocks/tick
        f64 move_x = -std::sin(yaw_rad) * speed;
        f64 move_z = std::cos(yaw_rad) * speed;

        // Add to position
        x_ += move_x;
        z_ += move_z;
    }

    // Simple bounds check - keep mobs within reasonable range of spawn
    constexpr f64 max_distance = 100.0;
    if (std::abs(x_) > max_distance) x_ = std::copysign(max_distance, x_);
    if (std::abs(z_) > max_distance) z_ = std::copysign(max_distance, z_);
}

void Mob::apply_knockback(f64 source_x, f64 source_z, f32 strength) {
    // Implement original Minecraft knockback algorithm
    // Calculate direction away from source
    f64 dx = x_ - source_x;
    f64 dz = z_ - source_z;
    f64 dist = std::sqrt(dx * dx + dz * dz);

    if (dist < 0.01) {
        // Too close, use small value to avoid division by zero
        dist = 0.01;
    }

    // Step 1: Halve all existing motion
    movement_.velocity_x /= 2.0;
    movement_.velocity_y /= 2.0;
    movement_.velocity_z /= 2.0;

    // Step 2: Apply knockback in horizontal direction (subtract because we want to push away)
    // Original uses strength = 0.4F
    movement_.velocity_x -= (dx / dist) * static_cast<f64>(strength);
    movement_.velocity_z -= (dz / dist) * static_cast<f64>(strength);

    // Step 3: Add upward boost
    movement_.velocity_y += 0.4;

    // Step 4: Cap upward velocity
    if (movement_.velocity_y > 0.4) {
        movement_.velocity_y = 0.4;
    }
}

void Mob::on_attacked_by(f64 attacker_x, f64 attacker_z) {
    // Set panic mode for passive mobs
    if (!is_hostile()) {
        panic_timer_ = 120;  // Panic for 6 seconds (120 ticks)
        flee_from_x_ = attacker_x;
        flee_from_z_ = attacker_z;
    }
}

std::vector<std::pair<i16, i8>> Mob::get_death_drops() const {
    std::vector<std::pair<i16, i8>> drops;

    static std::random_device rd;
    static std::mt19937 gen(rd());
    static std::uniform_int_distribution<> drop_count(0, 2);

    switch (mob_type_) {
        case MobType::Pig:
            // Pigs drop 0-2 raw porkchops (item ID 319)
            drops.push_back({static_cast<i16>(319), static_cast<i8>(drop_count(gen))});
            break;

        case MobType::Cow:
            // Cows drop 0-2 leather (item ID 334) and 0-2 raw beef (item ID 363)
            drops.push_back({static_cast<i16>(334), static_cast<i8>(drop_count(gen))});
            drops.push_back({static_cast<i16>(363), static_cast<i8>(drop_count(gen))});
            break;

        case MobType::Chicken:
            // Chickens drop 0-2 feathers (item ID 288) and 0-1 raw chicken (item ID 365)
            drops.push_back({static_cast<i16>(288), static_cast<i8>(drop_count(gen))});
            {
                std::uniform_int_distribution<> chicken_drop(0, 1);
                drops.push_back({static_cast<i16>(365), static_cast<i8>(chicken_drop(gen))});
            }
            break;

        case MobType::Sheep:
            // Sheep drop 1 wool (item ID 35) - color depends on metadata
            // Default to white wool
            drops.push_back({static_cast<i16>(35), static_cast<i8>(1)});
            break;

        case MobType::Zombie:
            // Zombies drop 0-2 feathers (should be rotten flesh but using feathers for Beta 1.7.3)
            drops.push_back({static_cast<i16>(288), static_cast<i8>(drop_count(gen))});
            break;

        case MobType::Skeleton:
            // Skeletons drop 0-2 arrows (item ID 262) and 0-2 bones (item ID 352)
            drops.push_back({static_cast<i16>(262), static_cast<i8>(drop_count(gen))});
            drops.push_back({static_cast<i16>(352), static_cast<i8>(drop_count(gen))});
            break;

        case MobType::Spider:
            // Spiders drop 0-2 string (item ID 287)
            drops.push_back({static_cast<i16>(287), static_cast<i8>(drop_count(gen))});
            break;

        case MobType::Creeper:
            // Creepers drop 0-2 gunpowder (item ID 289)
            drops.push_back({static_cast<i16>(289), static_cast<i8>(drop_count(gen))});
            break;

        default:
            break;
    }

    return drops;
}

} // namespace mcserver
