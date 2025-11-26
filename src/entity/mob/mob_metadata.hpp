#pragma once

#include "util/types.hpp"
#include <unordered_map>
#include <variant>
#include <string>
#include <vector>

namespace mcserver {

// Beta 1.7.3 metadata types
enum class MetadataType : u8 {
    Byte = 0,
    Short = 1,
    Int = 2,
    Float = 3,
    String = 4,
    ItemStack = 5,
    BlockPos = 6
};

// Metadata value variant
using MetadataValue = std::variant<i8, i16, i32, f32, std::string>;

// Metadata entry with index and typed value
struct MetadataEntry {
    u8 index;
    MetadataType type;
    MetadataValue value;
};

// Mob metadata manager (DataWatcher in original)
class MobMetadata {
public:
    MobMetadata() = default;

    // Set metadata values
    void set_byte(u8 index, i8 value);
    void set_short(u8 index, i16 value);
    void set_int(u8 index, i32 value);
    void set_float(u8 index, f32 value);
    void set_string(u8 index, const std::string& value);

    // Get metadata values
    i8 get_byte(u8 index, i8 default_value = 0) const;
    i16 get_short(u8 index, i16 default_value = 0) const;
    i32 get_int(u8 index, i32 default_value = 0) const;
    f32 get_float(u8 index, f32 default_value = 0.0f) const;
    std::string get_string(u8 index, const std::string& default_value = "") const;

    // Check if metadata exists
    bool has_metadata(u8 index) const;

    // Get all metadata entries for packet serialization
    const std::unordered_map<u8, MetadataEntry>& get_all() const { return entries_; }

    // Clear all metadata
    void clear() { entries_.clear(); }

private:
    std::unordered_map<u8, MetadataEntry> entries_;
};

// Sheep color constants (index 16, lower 4 bits)
enum class SheepColor : i8 {
    White = 0,
    Orange = 1,
    Magenta = 2,
    LightBlue = 3,
    Yellow = 4,
    Lime = 5,
    Pink = 6,
    Gray = 7,
    LightGray = 8,
    Cyan = 9,
    Purple = 10,
    Blue = 11,
    Brown = 12,
    Green = 13,
    Red = 14,
    Black = 15
};

// Wolf metadata indices
constexpr u8 WOLF_FLAGS_INDEX = 16;
constexpr u8 WOLF_OWNER_INDEX = 17;
constexpr u8 WOLF_HEALTH_INDEX = 18;

// Wolf flags (index 16)
constexpr i8 WOLF_FLAG_SITTING = 0x01;
constexpr i8 WOLF_FLAG_ANGRY = 0x02;
constexpr i8 WOLF_FLAG_TAMED = 0x04;

// Sheep metadata index
constexpr u8 SHEEP_COLOR_INDEX = 16;
constexpr i8 SHEEP_FLAG_SHEARED = 0x10;

} // namespace mcserver
