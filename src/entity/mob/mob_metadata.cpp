#include "mob_metadata.hpp"

namespace mcserver {

void MobMetadata::set_byte(u8 index, i8 value) {
    entries_[index] = MetadataEntry{index, MetadataType::Byte, value};
}

void MobMetadata::set_short(u8 index, i16 value) {
    entries_[index] = MetadataEntry{index, MetadataType::Short, value};
}

void MobMetadata::set_int(u8 index, i32 value) {
    entries_[index] = MetadataEntry{index, MetadataType::Int, value};
}

void MobMetadata::set_float(u8 index, f32 value) {
    entries_[index] = MetadataEntry{index, MetadataType::Float, value};
}

void MobMetadata::set_string(u8 index, const std::string& value) {
    entries_[index] = MetadataEntry{index, MetadataType::String, value};
}

i8 MobMetadata::get_byte(u8 index, i8 default_value) const {
    auto it = entries_.find(index);
    if (it == entries_.end() || it->second.type != MetadataType::Byte) {
        return default_value;
    }
    return std::get<i8>(it->second.value);
}

i16 MobMetadata::get_short(u8 index, i16 default_value) const {
    auto it = entries_.find(index);
    if (it == entries_.end() || it->second.type != MetadataType::Short) {
        return default_value;
    }
    return std::get<i16>(it->second.value);
}

i32 MobMetadata::get_int(u8 index, i32 default_value) const {
    auto it = entries_.find(index);
    if (it == entries_.end() || it->second.type != MetadataType::Int) {
        return default_value;
    }
    return std::get<i32>(it->second.value);
}

f32 MobMetadata::get_float(u8 index, f32 default_value) const {
    auto it = entries_.find(index);
    if (it == entries_.end() || it->second.type != MetadataType::Float) {
        return default_value;
    }
    return std::get<f32>(it->second.value);
}

std::string MobMetadata::get_string(u8 index, const std::string& default_value) const {
    auto it = entries_.find(index);
    if (it == entries_.end() || it->second.type != MetadataType::String) {
        return default_value;
    }
    return std::get<std::string>(it->second.value);
}

bool MobMetadata::has_metadata(u8 index) const {
    return entries_.find(index) != entries_.end();
}

} // namespace mcserver
