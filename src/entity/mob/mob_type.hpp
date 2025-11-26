#pragma once

#include "util/types.hpp"

namespace mcserver {

// Mob types from Beta 1.7.3 EntityList
enum class MobType : i8 {
    // Generic base types
    Mob = 48,
    Monster = 49,

    // Hostile mobs
    Creeper = 50,
    Skeleton = 51,
    Spider = 52,
    Giant = 53,
    Zombie = 54,
    Slime = 55,
    Ghast = 56,
    PigZombie = 57,

    // Passive mobs
    Pig = 90,
    Sheep = 91,
    Cow = 92,
    Chicken = 93,
    Squid = 94,
    Wolf = 95
};

// Helper to check if a mob type is hostile
inline bool is_hostile_mob(MobType type) {
    return static_cast<i8>(type) >= 49 && static_cast<i8>(type) <= 57;
}

// Helper to check if a mob type is passive
inline bool is_passive_mob(MobType type) {
    return static_cast<i8>(type) >= 90 && static_cast<i8>(type) <= 95;
}

} // namespace mcserver
