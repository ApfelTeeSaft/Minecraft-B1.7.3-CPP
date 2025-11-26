#include "storage/nbt/nbt.hpp"

namespace mcserver {

// Clone implementations
std::unique_ptr<NBTTag> NBTByte::clone() const {
    return std::make_unique<NBTByte>(value);
}

std::unique_ptr<NBTTag> NBTShort::clone() const {
    return std::make_unique<NBTShort>(value);
}

std::unique_ptr<NBTTag> NBTInt::clone() const {
    return std::make_unique<NBTInt>(value);
}

std::unique_ptr<NBTTag> NBTLong::clone() const {
    return std::make_unique<NBTLong>(value);
}

std::unique_ptr<NBTTag> NBTFloat::clone() const {
    return std::make_unique<NBTFloat>(value);
}

std::unique_ptr<NBTTag> NBTDouble::clone() const {
    return std::make_unique<NBTDouble>(value);
}

std::unique_ptr<NBTTag> NBTByteArray::clone() const {
    return std::make_unique<NBTByteArray>(value);
}

std::unique_ptr<NBTTag> NBTString::clone() const {
    return std::make_unique<NBTString>(value);
}

std::unique_ptr<NBTTag> NBTList::clone() const {
    auto list = std::make_unique<NBTList>(element_type);
    for (const auto& tag : value) {
        list->value.push_back(tag->clone());
    }
    return list;
}

std::unique_ptr<NBTTag> NBTCompound::clone() const {
    auto compound = std::make_unique<NBTCompound>();
    for (const auto& [name, tag] : tags) {
        compound->tags[name] = tag->clone();
    }
    return compound;
}

// NBTList methods
void NBTList::add(std::unique_ptr<NBTTag> tag) {
    if (value.empty() && element_type == NBTType::End) {
        element_type = tag->get_type();
    }
    value.push_back(std::move(tag));
}

// NBTCompound set methods
void NBTCompound::set_byte(const std::string& name, i8 value) {
    tags[name] = std::make_unique<NBTByte>(value);
}

void NBTCompound::set_short(const std::string& name, i16 value) {
    tags[name] = std::make_unique<NBTShort>(value);
}

void NBTCompound::set_int(const std::string& name, i32 value) {
    tags[name] = std::make_unique<NBTInt>(value);
}

void NBTCompound::set_long(const std::string& name, i64 value) {
    tags[name] = std::make_unique<NBTLong>(value);
}

void NBTCompound::set_float(const std::string& name, f32 value) {
    tags[name] = std::make_unique<NBTFloat>(value);
}

void NBTCompound::set_double(const std::string& name, f64 value) {
    tags[name] = std::make_unique<NBTDouble>(value);
}

void NBTCompound::set_byte_array(const std::string& name, std::vector<i8> value) {
    tags[name] = std::make_unique<NBTByteArray>(std::move(value));
}

void NBTCompound::set_byte_array(const std::string& name, const i8* data, usize size) {
    tags[name] = std::make_unique<NBTByteArray>(data, size);
}

void NBTCompound::set_string(const std::string& name, std::string value) {
    tags[name] = std::make_unique<NBTString>(std::move(value));
}

void NBTCompound::set_bool(const std::string& name, bool value) {
    tags[name] = std::make_unique<NBTByte>(value ? 1 : 0);
}

void NBTCompound::set_tag(const std::string& name, std::unique_ptr<NBTTag> tag) {
    tags[name] = std::move(tag);
}

// NBTCompound has_key
bool NBTCompound::has_key(const std::string& name) const {
    return tags.find(name) != tags.end();
}

// NBTCompound get_tag
NBTTag* NBTCompound::get_tag(const std::string& name) const {
    auto it = tags.find(name);
    if (it != tags.end()) {
        return it->second.get();
    }
    return nullptr;
}

// NBTCompound get methods
Result<i8> NBTCompound::get_byte(const std::string& name) const {
    auto* tag = get_tag(name);
    if (!tag) {
        return ErrorCode::NotFound;
    }
    if (tag->get_type() != NBTType::Byte) {
        return ErrorCode::InvalidArgument;
    }
    return static_cast<NBTByte*>(tag)->value;
}

Result<i16> NBTCompound::get_short(const std::string& name) const {
    auto* tag = get_tag(name);
    if (!tag) {
        return ErrorCode::NotFound;
    }
    if (tag->get_type() != NBTType::Short) {
        return ErrorCode::InvalidArgument;
    }
    return static_cast<NBTShort*>(tag)->value;
}

Result<i32> NBTCompound::get_int(const std::string& name) const {
    auto* tag = get_tag(name);
    if (!tag) {
        return ErrorCode::NotFound;
    }
    if (tag->get_type() != NBTType::Int) {
        return ErrorCode::InvalidArgument;
    }
    return static_cast<NBTInt*>(tag)->value;
}

Result<i64> NBTCompound::get_long(const std::string& name) const {
    auto* tag = get_tag(name);
    if (!tag) {
        return ErrorCode::NotFound;
    }
    if (tag->get_type() != NBTType::Long) {
        return ErrorCode::InvalidArgument;
    }
    return static_cast<NBTLong*>(tag)->value;
}

Result<f32> NBTCompound::get_float(const std::string& name) const {
    auto* tag = get_tag(name);
    if (!tag) {
        return ErrorCode::NotFound;
    }
    if (tag->get_type() != NBTType::Float) {
        return ErrorCode::InvalidArgument;
    }
    return static_cast<NBTFloat*>(tag)->value;
}

Result<f64> NBTCompound::get_double(const std::string& name) const {
    auto* tag = get_tag(name);
    if (!tag) {
        return ErrorCode::NotFound;
    }
    if (tag->get_type() != NBTType::Double) {
        return ErrorCode::InvalidArgument;
    }
    return static_cast<NBTDouble*>(tag)->value;
}

Result<std::vector<i8>> NBTCompound::get_byte_array(const std::string& name) const {
    auto* tag = get_tag(name);
    if (!tag) {
        return ErrorCode::NotFound;
    }
    if (tag->get_type() != NBTType::ByteArray) {
        return ErrorCode::InvalidArgument;
    }
    return static_cast<NBTByteArray*>(tag)->value;
}

Result<std::string> NBTCompound::get_string(const std::string& name) const {
    auto* tag = get_tag(name);
    if (!tag) {
        return ErrorCode::NotFound;
    }
    if (tag->get_type() != NBTType::String) {
        return ErrorCode::InvalidArgument;
    }
    return static_cast<NBTString*>(tag)->value;
}

Result<bool> NBTCompound::get_bool(const std::string& name) const {
    auto result = get_byte(name);
    if (!result) {
        return result.error();
    }
    return result.value() != 0;
}

NBTCompound* NBTCompound::get_compound(const std::string& name) const {
    auto* tag = get_tag(name);
    if (tag && tag->get_type() == NBTType::Compound) {
        return static_cast<NBTCompound*>(tag);
    }
    return nullptr;
}

NBTList* NBTCompound::get_list(const std::string& name) const {
    auto* tag = get_tag(name);
    if (tag && tag->get_type() == NBTType::List) {
        return static_cast<NBTList*>(tag);
    }
    return nullptr;
}

} // namespace mcserver
