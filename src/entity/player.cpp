#include "player.hpp"
#include <algorithm>

namespace mcserver {

Player::Player(std::string username, i32 entity_id)
    : username_(std::move(username))
    , entity_id_(entity_id)
    , uuid_(UUID::from_string(username_)) {}

void Player::set_position(f64 x, f64 y, f64 z) {
    x_ = x;
    y_ = y;
    z_ = z;
}

void Player::set_rotation(f32 yaw, f32 pitch) {
    yaw_ = yaw;
    pitch_ = pitch;
}

void Player::set_health(i16 health) {
    health_ = std::clamp(health, static_cast<i16>(0), static_cast<i16>(20));
    if (health_change_callback_) {
        health_change_callback_(entity_id_, health_, false);
    }
}

void Player::take_damage(i16 damage) {
    if (damage <= 0 || health_ <= 0) {
        return;  // Already dead or invalid damage
    }

    health_ -= damage;
    bool just_died = false;
    if (health_ <= 0) {
        health_ = 0;
        just_died = true;
    }

    // Notify about health change (damage taken)
    if (health_change_callback_) {
        health_change_callback_(entity_id_, health_, true);
    }

    // Notify about death
    if (just_died && death_callback_) {
        death_callback_(entity_id_);
    }
}

void Player::heal(i16 amount) {
    if (amount <= 0 || health_ <= 0) {
        return;  // Dead or invalid amount
    }

    health_ += amount;
    if (health_ > 20) {
        health_ = 20;
    }

    // Notify about health change (healing)
    if (health_change_callback_) {
        health_change_callback_(entity_id_, health_, false);
    }
}

void Player::respawn(f64 spawn_x, f64 spawn_y, f64 spawn_z) {
    // Reset position to spawn
    x_ = spawn_x;
    y_ = spawn_y;
    z_ = spawn_z;

    // Reset rotation
    yaw_ = 0.0f;
    pitch_ = 0.0f;
    on_ground_ = false;

    // Reset health and food
    health_ = 20;
    food_ = 20;

    // Notify about health change (respawn healing)
    if (health_change_callback_) {
        health_change_callback_(entity_id_, health_, false);
    }
}

} // namespace mcserver
