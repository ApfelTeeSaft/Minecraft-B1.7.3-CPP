#include "entity/mob/passive_mob.hpp"
#include <random>

namespace mcserver {

// Pig implementation
MobPig::MobPig(i32 entity_id)
    : Mob(entity_id, MobType::Pig) {
    health_ = 10;
    max_health_ = 10;
}

void MobPig::update() {
    Mob::update();
    // TODO: Add wandering AI, sounds, etc.
}

// Sheep implementation
MobSheep::MobSheep(i32 entity_id)
    : Mob(entity_id, MobType::Sheep) {
    health_ = 8;
    max_health_ = 8;

    // Random sheep color (weighted towards common colors)
    static std::random_device rd;
    static std::mt19937 gen(rd());
    static std::discrete_distribution<> color_dist({
        81,  // White (most common)
        1,   // Orange
        1,   // Magenta
        1,   // Light Blue
        1,   // Yellow
        1,   // Lime
        8,   // Pink
        10,  // Gray
        10,  // Light Gray
        1,   // Cyan
        1,   // Purple
        1,   // Blue
        7,   // Brown
        1,   // Green
        1,   // Red
        10   // Black
    });

    i32 color_index = color_dist(gen);
    set_color(static_cast<SheepColor>(color_index));
}

void MobSheep::update() {
    Mob::update();
    // TODO: Add wandering AI, sounds, etc.
}

void MobSheep::set_color(SheepColor color) {
    i8 current = metadata_.get_byte(SHEEP_COLOR_INDEX, 0);
    i8 color_value = static_cast<i8>(color) & 0x0F;  // Lower 4 bits
    i8 sheared_bit = current & SHEEP_FLAG_SHEARED;   // Preserve sheared flag
    metadata_.set_byte(SHEEP_COLOR_INDEX, color_value | sheared_bit);
}

SheepColor MobSheep::get_color() const {
    i8 value = metadata_.get_byte(SHEEP_COLOR_INDEX, 0);
    return static_cast<SheepColor>(value & 0x0F);
}

void MobSheep::set_sheared(bool sheared) {
    i8 current = metadata_.get_byte(SHEEP_COLOR_INDEX, 0);
    i8 color_bits = current & 0x0F;  // Preserve color
    i8 sheared_bit = sheared ? SHEEP_FLAG_SHEARED : 0;
    metadata_.set_byte(SHEEP_COLOR_INDEX, color_bits | sheared_bit);
}

bool MobSheep::is_sheared() const {
    i8 value = metadata_.get_byte(SHEEP_COLOR_INDEX, 0);
    return (value & SHEEP_FLAG_SHEARED) != 0;
}

// Cow implementation
MobCow::MobCow(i32 entity_id)
    : Mob(entity_id, MobType::Cow) {
    health_ = 10;
    max_health_ = 10;
}

void MobCow::update() {
    Mob::update();
    // TODO: Add wandering AI, sounds, etc.
}

// Chicken implementation
MobChicken::MobChicken(i32 entity_id)
    : Mob(entity_id, MobType::Chicken) {
    health_ = 4;
    max_health_ = 4;
}

void MobChicken::update() {
    Mob::update();
    // TODO: Add wandering AI, sounds, etc.
}

} // namespace mcserver
