#pragma once

#include "storage/nbt/nbt.hpp"
#include "util/result.hpp"
#include <vector>
#include <iosfwd>

namespace mcserver {

// NBT I/O utilities for reading and writing NBT data
class NBTReader {
public:
    explicit NBTReader(const std::vector<u8>& data);
    explicit NBTReader(const u8* data, usize size);

    // Read a named NBT compound (root tag)
    Result<std::unique_ptr<NBTCompound>> read_compound();

    // Read a tag (without name)
    Result<std::unique_ptr<NBTTag>> read_tag(NBTType type);

private:
    Result<std::string> read_string();
    Result<i8> read_byte();
    Result<i16> read_short();
    Result<i32> read_int();
    Result<i64> read_long();
    Result<f32> read_float();
    Result<f64> read_double();

    const u8* data_;
    usize size_;
    usize pos_ = 0;

    bool can_read(usize bytes) const { return pos_ + bytes <= size_; }
};

class NBTWriter {
public:
    NBTWriter();

    // Write a named NBT compound (root tag)
    void write_compound(const std::string& name, const NBTCompound& compound);

    // Write a tag with name
    void write_tag(const std::string& name, const NBTTag& tag);

    // Get the written data
    const std::vector<u8>& data() const { return data_; }
    std::vector<u8> take_data() { return std::move(data_); }

private:
    void write_string(const std::string& str);
    void write_byte(i8 value);
    void write_short(i16 value);
    void write_int(i32 value);
    void write_long(i64 value);
    void write_float(f32 value);
    void write_double(f64 value);
    void write_type(NBTType type);

    void write_tag_impl(const NBTTag& tag);

    std::vector<u8> data_;
};

// Compression utilities
namespace nbt_compression {

// Compress data using zlib (deflate)
Result<std::vector<u8>> compress_zlib(const std::vector<u8>& data);

// Decompress data using zlib (inflate)
Result<std::vector<u8>> decompress_zlib(const u8* data, usize size);

// Compress data using gzip
Result<std::vector<u8>> compress_gzip(const std::vector<u8>& data);

// Decompress data using gzip
Result<std::vector<u8>> decompress_gzip(const u8* data, usize size);

} // namespace nbt_compression

} // namespace mcserver
