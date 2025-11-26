#pragma once

#include "util/types.hpp"

namespace mcserver {

// AI states for mob behavior
enum class MobAIState {
    Idle,       // Standing still
    Wandering,  // Random movement
    Chasing,    // Following a target
    Attacking,  // In combat
    Fleeing     // Running away
};

// AI goals and behaviors
struct MobAIGoal {
    virtual ~MobAIGoal() = default;
    virtual bool should_execute() = 0;
    virtual void start() = 0;
    virtual void update() = 0;
    virtual void stop() = 0;
};

// Movement parameters for mobs
struct MobMovement {
    f64 velocity_x = 0.0;
    f64 velocity_y = 0.0;
    f64 velocity_z = 0.0;

    f32 move_speed = 0.0f;      // Blocks per second
    f32 target_yaw = 0.0f;      // Desired yaw

    bool is_moving = false;
    bool on_ground = true;

    i32 idle_ticks = 0;         // Ticks spent idle
    i32 wander_ticks = 0;       // Ticks spent wandering

    void reset() {
        velocity_x = velocity_y = velocity_z = 0.0;
        is_moving = false;
        idle_ticks = wander_ticks = 0;
    }
};

} // namespace mcserver
