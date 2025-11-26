#pragma once

#include "util/types.hpp"
#include "entity/inventory/item_stack.hpp"
#include <memory>

namespace mcserver {

// ItemEntity represents a dropped item in the world
class ItemEntity {
public:
    ItemEntity(i32 entity_id, const ItemStack& item, f64 x, f64 y, f64 z)
        : entity_id_(entity_id)
        , item_(std::make_unique<ItemStack>(item))
        , x_(x)
        , y_(y)
        , z_(z)
        , velocity_x_(0.0)
        , velocity_y_(0.0)
        , velocity_z_(0.0)
        , age_(0)
        , pickup_delay_(10) {}  // 10 ticks = 0.5 seconds delay

    // Getters
    i32 get_entity_id() const { return entity_id_; }
    const ItemStack* get_item() const { return item_.get(); }
    f64 get_x() const { return x_; }
    f64 get_y() const { return y_; }
    f64 get_z() const { return z_; }
    f64 get_velocity_x() const { return velocity_x_; }
    f64 get_velocity_y() const { return velocity_y_; }
    f64 get_velocity_z() const { return velocity_z_; }
    i32 get_age() const { return age_; }
    i32 get_pickup_delay() const { return pickup_delay_; }

    // Setters
    void set_position(f64 x, f64 y, f64 z) {
        x_ = x;
        y_ = y;
        z_ = z;
    }

    void set_velocity(f64 vx, f64 vy, f64 vz) {
        velocity_x_ = vx;
        velocity_y_ = vy;
        velocity_z_ = vz;
    }

    void set_pickup_delay(i32 delay) {
        pickup_delay_ = delay;
    }

    // Check if item can be picked up
    bool can_be_picked_up() const {
        return pickup_delay_ <= 0;
    }

    // Check if item should despawn (5 minutes = 6000 ticks)
    bool should_despawn() const {
        return age_ >= 6000;
    }

    // Update item (called every tick)
    void tick() {
        age_++;

        if (pickup_delay_ > 0) {
            pickup_delay_--;
        }

        // Apply gravity
        velocity_y_ -= 0.04;  // Gravity acceleration

        // Apply velocity
        x_ += velocity_x_;
        y_ += velocity_y_;
        z_ += velocity_z_;

        // Apply friction/drag
        velocity_x_ *= 0.98;
        velocity_y_ *= 0.98;
        velocity_z_ *= 0.98;

        // Ground collision (simple check)
        if (y_ < 1.0) {
            y_ = 1.0;
            velocity_y_ = 0.0;
            velocity_x_ *= 0.5;  // Friction on ground
            velocity_z_ *= 0.5;
        }
    }

    // Check if entity is within pickup range of a position
    bool is_in_pickup_range(f64 px, f64 py, f64 pz, f64 range = 1.0) const {
        f64 dx = x_ - px;
        f64 dy = y_ - py;
        f64 dz = z_ - pz;
        f64 dist_sq = dx * dx + dy * dy + dz * dz;
        return dist_sq <= range * range;
    }

private:
    i32 entity_id_;
    std::unique_ptr<ItemStack> item_;
    f64 x_, y_, z_;
    f64 velocity_x_, velocity_y_, velocity_z_;
    i32 age_;           // Ticks since spawn
    i32 pickup_delay_;  // Ticks until can be picked up
};

} // namespace mcserver
