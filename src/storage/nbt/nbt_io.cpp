#include "storage/nbt/nbt_io.hpp"
#include <cstring>
#include <zlib.h>
#include <algorithm>

namespace mcserver {

// Helper to convert bytes to network byte order (big-endian)
static i16 read_i16_be(const u8* data) {
    return static_cast<i16>((data[0] << 8) | data[1]);
}

static i32 read_i32_be(const u8* data) {
    return static_cast<i32>((data[0] << 24) | (data[1] << 16) | (data[2] << 8) | data[3]);
}

static i64 read_i64_be(const u8* data) {
    return (static_cast<i64>(data[0]) << 56) |
           (static_cast<i64>(data[1]) << 48) |
           (static_cast<i64>(data[2]) << 40) |
           (static_cast<i64>(data[3]) << 32) |
           (static_cast<i64>(data[4]) << 24) |
           (static_cast<i64>(data[5]) << 16) |
           (static_cast<i64>(data[6]) << 8) |
            static_cast<i64>(data[7]);
}

static void write_i16_be(std::vector<u8>& data, i16 value) {
    data.push_back(static_cast<u8>((value >> 8) & 0xFF));
    data.push_back(static_cast<u8>(value & 0xFF));
}

static void write_i32_be(std::vector<u8>& data, i32 value) {
    data.push_back(static_cast<u8>((value >> 24) & 0xFF));
    data.push_back(static_cast<u8>((value >> 16) & 0xFF));
    data.push_back(static_cast<u8>((value >> 8) & 0xFF));
    data.push_back(static_cast<u8>(value & 0xFF));
}

static void write_i64_be(std::vector<u8>& data, i64 value) {
    data.push_back(static_cast<u8>((value >> 56) & 0xFF));
    data.push_back(static_cast<u8>((value >> 48) & 0xFF));
    data.push_back(static_cast<u8>((value >> 40) & 0xFF));
    data.push_back(static_cast<u8>((value >> 32) & 0xFF));
    data.push_back(static_cast<u8>((value >> 24) & 0xFF));
    data.push_back(static_cast<u8>((value >> 16) & 0xFF));
    data.push_back(static_cast<u8>((value >> 8) & 0xFF));
    data.push_back(static_cast<u8>(value & 0xFF));
}

// NBTReader implementation
NBTReader::NBTReader(const std::vector<u8>& data)
    : data_(data.data()), size_(data.size()), pos_(0) {}

NBTReader::NBTReader(const u8* data, usize size)
    : data_(data), size_(size), pos_(0) {}

Result<i8> NBTReader::read_byte() {
    if (!can_read(1)) {
        return ErrorCode::InvalidArgument;
    }
    return static_cast<i8>(data_[pos_++]);
}

Result<i16> NBTReader::read_short() {
    if (!can_read(2)) {
        return ErrorCode::InvalidArgument;
    }
    i16 value = read_i16_be(data_ + pos_);
    pos_ += 2;
    return value;
}

Result<i32> NBTReader::read_int() {
    if (!can_read(4)) {
        return ErrorCode::InvalidArgument;
    }
    i32 value = read_i32_be(data_ + pos_);
    pos_ += 4;
    return value;
}

Result<i64> NBTReader::read_long() {
    if (!can_read(8)) {
        return ErrorCode::InvalidArgument;
    }
    i64 value = read_i64_be(data_ + pos_);
    pos_ += 8;
    return value;
}

Result<f32> NBTReader::read_float() {
    auto result = read_int();
    if (!result) {
        return result.error();
    }
    i32 bits = result.value();
    f32 value;
    std::memcpy(&value, &bits, sizeof(f32));
    return value;
}

Result<f64> NBTReader::read_double() {
    auto result = read_long();
    if (!result) {
        return result.error();
    }
    i64 bits = result.value();
    f64 value;
    std::memcpy(&value, &bits, sizeof(f64));
    return value;
}

Result<std::string> NBTReader::read_string() {
    auto length_result = read_short();
    if (!length_result) {
        return length_result.error();
    }
    i16 length = length_result.value();
    if (length < 0 || !can_read(static_cast<usize>(length))) {
        return ErrorCode::InvalidArgument;
    }
    std::string str(reinterpret_cast<const char*>(data_ + pos_), static_cast<usize>(length));
    pos_ += static_cast<usize>(length);
    return str;
}

Result<std::unique_ptr<NBTTag>> NBTReader::read_tag(NBTType type) {
    switch (type) {
        case NBTType::Byte: {
            auto result = read_byte();
            if (!result) return result.error();
            return std::unique_ptr<NBTTag>(std::make_unique<NBTByte>(result.value()));
        }
        case NBTType::Short: {
            auto result = read_short();
            if (!result) return result.error();
            return std::unique_ptr<NBTTag>(std::make_unique<NBTShort>(result.value()));
        }
        case NBTType::Int: {
            auto result = read_int();
            if (!result) return result.error();
            return std::unique_ptr<NBTTag>(std::make_unique<NBTInt>(result.value()));
        }
        case NBTType::Long: {
            auto result = read_long();
            if (!result) return result.error();
            return std::unique_ptr<NBTTag>(std::make_unique<NBTLong>(result.value()));
        }
        case NBTType::Float: {
            auto result = read_float();
            if (!result) return result.error();
            return std::unique_ptr<NBTTag>(std::make_unique<NBTFloat>(result.value()));
        }
        case NBTType::Double: {
            auto result = read_double();
            if (!result) return result.error();
            return std::unique_ptr<NBTTag>(std::make_unique<NBTDouble>(result.value()));
        }
        case NBTType::ByteArray: {
            auto length_result = read_int();
            if (!length_result) return length_result.error();
            i32 length = length_result.value();
            if (length < 0 || !can_read(static_cast<usize>(length))) {
                return ErrorCode::InvalidArgument;
            }
            std::vector<i8> bytes(length);
            std::memcpy(bytes.data(), data_ + pos_, static_cast<usize>(length));
            pos_ += static_cast<usize>(length);
            return std::unique_ptr<NBTTag>(std::make_unique<NBTByteArray>(std::move(bytes)));
        }
        case NBTType::String: {
            auto result = read_string();
            if (!result) return result.error();
            return std::unique_ptr<NBTTag>(std::make_unique<NBTString>(result.value()));
        }
        case NBTType::List: {
            auto type_result = read_byte();
            if (!type_result) return type_result.error();
            auto element_type = static_cast<NBTType>(type_result.value());

            auto length_result = read_int();
            if (!length_result) return length_result.error();
            i32 length = length_result.value();
            if (length < 0) {
                return ErrorCode::InvalidArgument;
            }

            auto list = std::make_unique<NBTList>(element_type);
            for (i32 i = 0; i < length; ++i) {
                auto tag_result = read_tag(element_type);
                if (!tag_result) return tag_result.error();
                list->add(std::move(tag_result.value()));
            }
            return std::unique_ptr<NBTTag>(std::move(list));
        }
        case NBTType::Compound: {
            auto compound = std::make_unique<NBTCompound>();
            while (true) {
                auto type_result = read_byte();
                if (!type_result) return type_result.error();
                auto tag_type = static_cast<NBTType>(type_result.value());

                if (tag_type == NBTType::End) {
                    break;
                }

                auto name_result = read_string();
                if (!name_result) return name_result.error();
                std::string name = name_result.value();

                auto tag_result = read_tag(tag_type);
                if (!tag_result) return tag_result.error();

                compound->set_tag(name, std::move(tag_result.value()));
            }
            return std::unique_ptr<NBTTag>(std::move(compound));
        }
        default:
            return ErrorCode::InvalidArgument;
    }
}

Result<std::unique_ptr<NBTCompound>> NBTReader::read_compound() {
    // Read root tag type (should be Compound)
    auto type_result = read_byte();
    if (!type_result) {
        return type_result.error();
    }
    if (static_cast<NBTType>(type_result.value()) != NBTType::Compound) {
        return ErrorCode::InvalidArgument;
    }

    // Read root tag name (usually empty or "")
    auto name_result = read_string();
    if (!name_result) {
        return name_result.error();
    }

    // Read the compound tag
    auto tag_result = read_tag(NBTType::Compound);
    if (!tag_result) {
        return tag_result.error();
    }

    return std::unique_ptr<NBTCompound>(
        static_cast<NBTCompound*>(tag_result.value().release())
    );
}

// NBTWriter implementation
NBTWriter::NBTWriter() {
    data_.reserve(8192);
}

void NBTWriter::write_type(NBTType type) {
    data_.push_back(static_cast<u8>(type));
}

void NBTWriter::write_byte(i8 value) {
    data_.push_back(static_cast<u8>(value));
}

void NBTWriter::write_short(i16 value) {
    write_i16_be(data_, value);
}

void NBTWriter::write_int(i32 value) {
    write_i32_be(data_, value);
}

void NBTWriter::write_long(i64 value) {
    write_i64_be(data_, value);
}

void NBTWriter::write_float(f32 value) {
    i32 bits;
    std::memcpy(&bits, &value, sizeof(i32));
    write_int(bits);
}

void NBTWriter::write_double(f64 value) {
    i64 bits;
    std::memcpy(&bits, &value, sizeof(i64));
    write_long(bits);
}

void NBTWriter::write_string(const std::string& str) {
    write_short(static_cast<i16>(str.size()));
    data_.insert(data_.end(), str.begin(), str.end());
}

void NBTWriter::write_tag_impl(const NBTTag& tag) {
    switch (tag.get_type()) {
        case NBTType::Byte:
            write_byte(static_cast<const NBTByte&>(tag).value);
            break;
        case NBTType::Short:
            write_short(static_cast<const NBTShort&>(tag).value);
            break;
        case NBTType::Int:
            write_int(static_cast<const NBTInt&>(tag).value);
            break;
        case NBTType::Long:
            write_long(static_cast<const NBTLong&>(tag).value);
            break;
        case NBTType::Float:
            write_float(static_cast<const NBTFloat&>(tag).value);
            break;
        case NBTType::Double:
            write_double(static_cast<const NBTDouble&>(tag).value);
            break;
        case NBTType::ByteArray: {
            const auto& bytes = static_cast<const NBTByteArray&>(tag).value;
            write_int(static_cast<i32>(bytes.size()));
            data_.insert(data_.end(),
                        reinterpret_cast<const u8*>(bytes.data()),
                        reinterpret_cast<const u8*>(bytes.data() + bytes.size()));
            break;
        }
        case NBTType::String:
            write_string(static_cast<const NBTString&>(tag).value);
            break;
        case NBTType::List: {
            const auto& list = static_cast<const NBTList&>(tag);
            write_type(list.element_type);
            write_int(static_cast<i32>(list.value.size()));
            for (const auto& element : list.value) {
                write_tag_impl(*element);
            }
            break;
        }
        case NBTType::Compound: {
            const auto& compound = static_cast<const NBTCompound&>(tag);
            for (const auto& [name, child_tag] : compound.tags) {
                write_type(child_tag->get_type());
                write_string(name);
                write_tag_impl(*child_tag);
            }
            write_type(NBTType::End);
            break;
        }
        default:
            break;
    }
}

void NBTWriter::write_tag(const std::string& name, const NBTTag& tag) {
    write_type(tag.get_type());
    write_string(name);
    write_tag_impl(tag);
}

void NBTWriter::write_compound(const std::string& name, const NBTCompound& compound) {
    write_type(NBTType::Compound);
    write_string(name);
    write_tag_impl(compound);
}

// Compression utilities
namespace nbt_compression {

Result<std::vector<u8>> compress_zlib(const std::vector<u8>& data) {
    uLongf compressed_size = compressBound(static_cast<uLong>(data.size()));
    std::vector<u8> compressed(compressed_size);

    int result = compress(compressed.data(), &compressed_size,
                         data.data(), static_cast<uLong>(data.size()));

    if (result != Z_OK) {
        return ErrorCode::IOError;
    }

    compressed.resize(compressed_size);
    return compressed;
}

Result<std::vector<u8>> decompress_zlib(const u8* data, usize size) {
    // Start with a reasonable buffer size
    usize decompressed_size = size * 4;
    std::vector<u8> decompressed(decompressed_size);

    z_stream stream{};
    stream.next_in = const_cast<u8*>(data);
    stream.avail_in = static_cast<uInt>(size);
    stream.next_out = decompressed.data();
    stream.avail_out = static_cast<uInt>(decompressed_size);

    if (inflateInit(&stream) != Z_OK) {
        return ErrorCode::IOError;
    }

    int result = inflate(&stream, Z_FINISH);
    if (result != Z_STREAM_END) {
        inflateEnd(&stream);
        return ErrorCode::IOError;
    }

    decompressed.resize(stream.total_out);
    inflateEnd(&stream);
    return decompressed;
}

Result<std::vector<u8>> compress_gzip(const std::vector<u8>& data) {
    uLongf compressed_size = compressBound(static_cast<uLong>(data.size())) + 18;
    std::vector<u8> compressed(compressed_size);

    z_stream stream{};
    stream.next_in = const_cast<u8*>(data.data());
    stream.avail_in = static_cast<uInt>(data.size());
    stream.next_out = compressed.data();
    stream.avail_out = static_cast<uInt>(compressed_size);

    if (deflateInit2(&stream, Z_DEFAULT_COMPRESSION, Z_DEFLATED,
                     15 + 16, 8, Z_DEFAULT_STRATEGY) != Z_OK) {
        return ErrorCode::IOError;
    }

    int result = deflate(&stream, Z_FINISH);
    if (result != Z_STREAM_END) {
        deflateEnd(&stream);
        return ErrorCode::IOError;
    }

    compressed.resize(stream.total_out);
    deflateEnd(&stream);
    return compressed;
}

Result<std::vector<u8>> decompress_gzip(const u8* data, usize size) {
    usize decompressed_size = size * 4;
    std::vector<u8> decompressed(decompressed_size);

    z_stream stream{};
    stream.next_in = const_cast<u8*>(data);
    stream.avail_in = static_cast<uInt>(size);
    stream.next_out = decompressed.data();
    stream.avail_out = static_cast<uInt>(decompressed_size);

    if (inflateInit2(&stream, 15 + 16) != Z_OK) {
        return ErrorCode::IOError;
    }

    int result = inflate(&stream, Z_FINISH);
    if (result != Z_STREAM_END) {
        inflateEnd(&stream);
        return ErrorCode::IOError;
    }

    decompressed.resize(stream.total_out);
    inflateEnd(&stream);
    return decompressed;
}

} // namespace nbt_compression

} // namespace mcserver
