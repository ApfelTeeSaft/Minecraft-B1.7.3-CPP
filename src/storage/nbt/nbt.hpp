#pragma once

#include "util/types.hpp"
#include "util/result.hpp"
#include <string>
#include <vector>
#include <memory>
#include <unordered_map>

namespace mcserver {

// NBT Tag Types (Minecraft Beta 1.7.3)
enum class NBTType : u8 {
    End = 0,
    Byte = 1,
    Short = 2,
    Int = 3,
    Long = 4,
    Float = 5,
    Double = 6,
    ByteArray = 7,
    String = 8,
    List = 9,
    Compound = 10
};

// Forward declarations
class NBTTag;
class NBTCompound;

// Base class for all NBT tags
class NBTTag {
public:
    virtual ~NBTTag() = default;
    virtual NBTType get_type() const = 0;
    virtual std::unique_ptr<NBTTag> clone() const = 0;
};

// NBT Byte tag
class NBTByte : public NBTTag {
public:
    i8 value = 0;

    NBTByte() = default;
    explicit NBTByte(i8 v) : value(v) {}

    NBTType get_type() const override { return NBTType::Byte; }
    std::unique_ptr<NBTTag> clone() const override;
};

// NBT Short tag
class NBTShort : public NBTTag {
public:
    i16 value = 0;

    NBTShort() = default;
    explicit NBTShort(i16 v) : value(v) {}

    NBTType get_type() const override { return NBTType::Short; }
    std::unique_ptr<NBTTag> clone() const override;
};

// NBT Int tag
class NBTInt : public NBTTag {
public:
    i32 value = 0;

    NBTInt() = default;
    explicit NBTInt(i32 v) : value(v) {}

    NBTType get_type() const override { return NBTType::Int; }
    std::unique_ptr<NBTTag> clone() const override;
};

// NBT Long tag
class NBTLong : public NBTTag {
public:
    i64 value = 0;

    NBTLong() = default;
    explicit NBTLong(i64 v) : value(v) {}

    NBTType get_type() const override { return NBTType::Long; }
    std::unique_ptr<NBTTag> clone() const override;
};

// NBT Float tag
class NBTFloat : public NBTTag {
public:
    f32 value = 0.0f;

    NBTFloat() = default;
    explicit NBTFloat(f32 v) : value(v) {}

    NBTType get_type() const override { return NBTType::Float; }
    std::unique_ptr<NBTTag> clone() const override;
};

// NBT Double tag
class NBTDouble : public NBTTag {
public:
    f64 value = 0.0;

    NBTDouble() = default;
    explicit NBTDouble(f64 v) : value(v) {}

    NBTType get_type() const override { return NBTType::Double; }
    std::unique_ptr<NBTTag> clone() const override;
};

// NBT Byte Array tag
class NBTByteArray : public NBTTag {
public:
    std::vector<i8> value;

    NBTByteArray() = default;
    explicit NBTByteArray(std::vector<i8> v) : value(std::move(v)) {}
    explicit NBTByteArray(const i8* data, usize size) : value(data, data + size) {}

    NBTType get_type() const override { return NBTType::ByteArray; }
    std::unique_ptr<NBTTag> clone() const override;
};

// NBT String tag
class NBTString : public NBTTag {
public:
    std::string value;

    NBTString() = default;
    explicit NBTString(std::string v) : value(std::move(v)) {}

    NBTType get_type() const override { return NBTType::String; }
    std::unique_ptr<NBTTag> clone() const override;
};

// NBT List tag
class NBTList : public NBTTag {
public:
    NBTType element_type = NBTType::End;
    std::vector<std::unique_ptr<NBTTag>> value;

    NBTList() = default;
    explicit NBTList(NBTType type) : element_type(type) {}

    NBTType get_type() const override { return NBTType::List; }
    std::unique_ptr<NBTTag> clone() const override;

    void add(std::unique_ptr<NBTTag> tag);
    usize size() const { return value.size(); }
};

// NBT Compound tag (dictionary of named tags)
class NBTCompound : public NBTTag {
public:
    std::unordered_map<std::string, std::unique_ptr<NBTTag>> tags;

    NBTCompound() = default;

    NBTType get_type() const override { return NBTType::Compound; }
    std::unique_ptr<NBTTag> clone() const override;

    // Set methods
    void set_byte(const std::string& name, i8 value);
    void set_short(const std::string& name, i16 value);
    void set_int(const std::string& name, i32 value);
    void set_long(const std::string& name, i64 value);
    void set_float(const std::string& name, f32 value);
    void set_double(const std::string& name, f64 value);
    void set_byte_array(const std::string& name, std::vector<i8> value);
    void set_byte_array(const std::string& name, const i8* data, usize size);
    void set_string(const std::string& name, std::string value);
    void set_bool(const std::string& name, bool value);
    void set_tag(const std::string& name, std::unique_ptr<NBTTag> tag);

    // Get methods
    bool has_key(const std::string& name) const;
    NBTTag* get_tag(const std::string& name) const;

    Result<i8> get_byte(const std::string& name) const;
    Result<i16> get_short(const std::string& name) const;
    Result<i32> get_int(const std::string& name) const;
    Result<i64> get_long(const std::string& name) const;
    Result<f32> get_float(const std::string& name) const;
    Result<f64> get_double(const std::string& name) const;
    Result<std::vector<i8>> get_byte_array(const std::string& name) const;
    Result<std::string> get_string(const std::string& name) const;
    Result<bool> get_bool(const std::string& name) const;
    NBTCompound* get_compound(const std::string& name) const;
    NBTList* get_list(const std::string& name) const;
};

} // namespace mcserver
