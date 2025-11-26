#pragma once

#include "event.hpp"
#include "util/types.hpp"

namespace mcserver {

class Entity;
class Mob;

// Base class for all entity events
class EntityEvent : public Event {
public:
    explicit EntityEvent(Entity* entity) : entity_(entity) {}

    Entity* get_entity() const { return entity_; }

protected:
    Entity* entity_;
};

// Entity spawn event (cancellable)
class EntitySpawnEvent : public EntityEvent, public CancellableEvent {
public:
    explicit EntitySpawnEvent(Entity* entity) : EntityEvent(entity) {}

    const char* get_event_name() const override { return "EntitySpawnEvent"; }
};

// Entity death event
class EntityDeathEvent : public EntityEvent {
public:
    EntityDeathEvent(Entity* entity, Entity* killer = nullptr)
        : EntityEvent(entity), killer_(killer) {}

    const char* get_event_name() const override { return "EntityDeathEvent"; }

    Entity* get_killer() const { return killer_; }

    // Control drops
    bool should_drop_items() const { return drop_items_; }
    void set_drop_items(bool drop) { drop_items_ = drop; }

    // Drop experience
    i32 get_dropped_exp() const { return dropped_exp_; }
    void set_dropped_exp(i32 exp) { dropped_exp_ = exp; }

private:
    Entity* killer_;
    bool drop_items_ = true;
    i32 dropped_exp_ = 0;
};

// Entity damage event (cancellable)
class EntityDamageEvent : public EntityEvent, public CancellableEvent {
public:
    enum class DamageCause {
        CONTACT,         // Cactus
        ENTITY_ATTACK,   // Hit by entity
        PROJECTILE,      // Hit by arrow/snowball
        SUFFOCATION,     // In wall
        FALL,            // Fall damage
        FIRE,            // Standing in fire
        FIRE_TICK,       // Burning
        LAVA,            // In lava
        DROWNING,        // Out of air underwater
        BLOCK_EXPLOSION, // TNT
        ENTITY_EXPLOSION,// Creeper
        VOID,            // Falling into void
        LIGHTNING,       // Struck by lightning
        CUSTOM           // Plugin damage
    };

    EntityDamageEvent(Entity* entity, DamageCause cause, f32 damage)
        : EntityEvent(entity), cause_(cause), damage_(damage) {}

    const char* get_event_name() const override { return "EntityDamageEvent"; }

    DamageCause get_cause() const { return cause_; }
    f32 get_damage() const { return damage_; }
    void set_damage(f32 damage) { damage_ = damage; }

private:
    DamageCause cause_;
    f32 damage_;
};

// Entity damage by entity event (extends EntityDamageEvent)
class EntityDamageByEntityEvent : public EntityDamageEvent {
public:
    EntityDamageByEntityEvent(Entity* entity, Entity* damager, f32 damage)
        : EntityDamageEvent(entity, DamageCause::ENTITY_ATTACK, damage), damager_(damager) {}

    const char* get_event_name() const override { return "EntityDamageByEntityEvent"; }

    Entity* get_damager() const { return damager_; }

private:
    Entity* damager_;
};

// Entity target event (cancellable) - when mob targets something
class EntityTargetEvent : public EntityEvent, public CancellableEvent {
public:
    enum class TargetReason {
        TARGET_ATTACKED_ENTITY,  // Retaliation
        TARGET_ATTACKED_NEARBY_ENTITY,  // Defending
        CLOSEST_PLAYER,          // Found nearest player
        RANDOM_TARGET,           // Random targeting
        FORGOT_TARGET,           // Lost target
        CUSTOM                   // Plugin reason
    };

    EntityTargetEvent(Entity* entity, Entity* target, TargetReason reason)
        : EntityEvent(entity), target_(target), reason_(reason) {}

    const char* get_event_name() const override { return "EntityTargetEvent"; }

    Entity* get_target() const { return target_; }
    void set_target(Entity* target) { target_ = target; }

    TargetReason get_reason() const { return reason_; }

private:
    Entity* target_;
    TargetReason reason_;
};

} // namespace mcserver
