#pragma once

#include "entity/mob/mob_type.hpp"
#include "entity/mob/mob_ai.hpp"
#include "entity/mob/mob_metadata.hpp"
#include "util/types.hpp"
#include <string>
#include <functional>

namespace mcserver {

// Callback for mob movement broadcasting
using MobMoveCallback = std::function<void(i32 entity_id, f64 old_x, f64 old_y, f64 old_z,
                                           f64 new_x, f64 new_y, f64 new_z,
                                           f32 yaw, f32 pitch)>;

// Base mob entity class for all NPCs/mobs
class Mob {
public:
    Mob(i32 entity_id, MobType type);
    virtual ~Mob() = default;

    // Getters
    i32 get_entity_id() const { return entity_id_; }
    MobType get_mob_type() const { return mob_type_; }

    f64 get_x() const { return x_; }
    f64 get_y() const { return y_; }
    f64 get_z() const { return z_; }
    f32 get_yaw() const { return yaw_; }
    f32 get_pitch() const { return pitch_; }

    i16 get_health() const { return health_; }
    i16 get_max_health() const { return max_health_; }
    bool is_dead() const { return health_ <= 0; }

    MobAIState get_ai_state() const { return ai_state_; }
    i32 get_death_timer() const { return death_timer_; }

    // Setters
    void set_position(f64 x, f64 y, f64 z);
    void set_rotation(f32 yaw, f32 pitch);
    void set_health(i16 health);
    void set_move_callback(MobMoveCallback callback) { move_callback_ = callback; }

    // Combat methods
    void apply_knockback(f64 source_x, f64 source_z, f32 strength = 0.4f);
    void on_attacked_by(f64 attacker_x, f64 attacker_z);

    // Death handling
    bool should_despawn() const { return is_dead() && death_timer_ <= 0; }

    // Get items this mob drops on death
    virtual std::vector<std::pair<i16, i8>> get_death_drops() const;

    // Update tick (called every server tick)
    virtual void update();

    // Get the name of this mob type (for logging/debugging)
    virtual std::string get_name() const;

    // Metadata access
    MobMetadata* get_metadata() { return &metadata_; }
    const MobMetadata* get_metadata() const { return &metadata_; }

    // AI behavior methods (can be overridden by subclasses)
    virtual void update_ai();
    virtual bool is_hostile() const { return false; }
    virtual f32 get_movement_speed() const { return 0.2f; } // Blocks per second

protected:
    // Apply movement and velocity
    void apply_movement();

    // Random wandering behavior
    void wander_randomly();

    i32 entity_id_;
    MobType mob_type_;

    // Position
    f64 x_ = 0.0;
    f64 y_ = 64.0;
    f64 z_ = 0.0;
    f64 prev_x_ = 0.0;
    f64 prev_y_ = 64.0;
    f64 prev_z_ = 0.0;
    f32 yaw_ = 0.0f;
    f32 pitch_ = 0.0f;

    // Stats
    i16 health_ = 20;
    i16 max_health_ = 20;

    // Timers
    i32 age_ = 0;  // Age in ticks
    i32 panic_timer_ = 0;  // Ticks remaining in panic mode
    i32 death_timer_ = 40;  // Ticks before despawn after death (2 seconds)

    // Panic mode (fleeing when attacked)
    f64 flee_from_x_ = 0.0;
    f64 flee_from_z_ = 0.0;

    // AI state
    MobAIState ai_state_ = MobAIState::Idle;
    MobMovement movement_;

    // Metadata
    MobMetadata metadata_;

    // Callback for movement
    MobMoveCallback move_callback_;
};

} // namespace mcserver
