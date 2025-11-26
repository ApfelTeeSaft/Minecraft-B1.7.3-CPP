#pragma once

#include "entity/inventory/inventory.hpp"
#include "util/types.hpp"
#include "util/uuid.hpp"
#include <string>
#include <functional>

namespace mcserver {

// Callback for when player health changes
using PlayerHealthChangeCallback = std::function<void(i32 entity_id, i16 health, bool took_damage)>;
// Callback for when player dies (entity_id)
using PlayerDeathCallback = std::function<void(i32 entity_id)>;

// Player entity for tracking player state
class Player {
public:
    Player(std::string username, i32 entity_id);

    // Getters
    const std::string& get_username() const { return username_; }
    i32 get_entity_id() const { return entity_id_; }
    const UUID& get_uuid() const { return uuid_; }

    f64 get_x() const { return x_; }
    f64 get_y() const { return y_; }
    f64 get_z() const { return z_; }
    f32 get_yaw() const { return yaw_; }
    f32 get_pitch() const { return pitch_; }
    bool is_on_ground() const { return on_ground_; }

    i16 get_health() const { return health_; }
    i16 get_food() const { return food_; }
    bool is_dead() const { return health_ <= 0; }
    bool is_sneaking() const { return sneaking_; }
    bool is_sprinting() const { return sprinting_; }

    // Setters
    void set_position(f64 x, f64 y, f64 z);
    void set_rotation(f32 yaw, f32 pitch);
    void set_on_ground(bool on_ground) { on_ground_ = on_ground; }
    void set_sneaking(bool sneaking) { sneaking_ = sneaking; }
    void set_sprinting(bool sprinting) { sprinting_ = sprinting; }
    void set_health(i16 health);
    void set_food(i16 food) { food_ = food; }

    // Damage and combat
    void take_damage(i16 damage);
    void heal(i16 amount);
    void respawn(f64 spawn_x, f64 spawn_y, f64 spawn_z);

    // Callbacks
    void set_health_change_callback(PlayerHealthChangeCallback callback) {
        health_change_callback_ = callback;
    }

    void set_death_callback(PlayerDeathCallback callback) {
        death_callback_ = callback;
    }

    // Inventory access
    Inventory* get_inventory() { return &inventory_; }
    const Inventory* get_inventory() const { return &inventory_; }

private:
    std::string username_;
    i32 entity_id_;
    UUID uuid_;  // Unique identifier for this player

    // Position
    f64 x_ = 0.0;
    f64 y_ = 64.0;
    f64 z_ = 0.0;
    f32 yaw_ = 0.0f;
    f32 pitch_ = 0.0f;
    bool on_ground_ = false;

    // Stats
    i16 health_ = 20;  // 20 = full health (10 hearts)
    i16 food_ = 20;    // 20 = full food

    // Actions
    bool sneaking_ = false;
    bool sprinting_ = false;

    // Callbacks
    PlayerHealthChangeCallback health_change_callback_;
    PlayerDeathCallback death_callback_;

    // Inventory
    Inventory inventory_;
};

} // namespace mcserver
