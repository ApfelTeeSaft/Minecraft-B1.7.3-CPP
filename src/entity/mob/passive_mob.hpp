#pragma once

#include "entity/mob/mob.hpp"

namespace mcserver {

// Pig mob (ID 90)
class MobPig : public Mob {
public:
    explicit MobPig(i32 entity_id);
    void update() override;
};

// Sheep mob (ID 91)
class MobSheep : public Mob {
public:
    explicit MobSheep(i32 entity_id);
    void update() override;

    // Set sheep color (0-15)
    void set_color(SheepColor color);
    SheepColor get_color() const;

    // Sheared status
    void set_sheared(bool sheared);
    bool is_sheared() const;
};

// Cow mob (ID 92)
class MobCow : public Mob {
public:
    explicit MobCow(i32 entity_id);
    void update() override;
};

// Chicken mob (ID 93)
class MobChicken : public Mob {
public:
    explicit MobChicken(i32 entity_id);
    void update() override;
};

} // namespace mcserver
