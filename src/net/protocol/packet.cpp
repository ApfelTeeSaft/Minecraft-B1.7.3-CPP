#include "packet.hpp"
#include <cstring>
#include <algorithm>

namespace mcserver {

PacketBuffer::PacketBuffer(std::vector<byte> data)
    : data_(std::move(data)), position_(0) {}

bool PacketBuffer::ensure_available(usize bytes) {
    return position_ + bytes <= data_.size();
}

Result<u8> PacketBuffer::read_u8() {
    if (!ensure_available(1)) {
        return ErrorCode::ParseError;
    }
    u8 value = static_cast<u8>(data_[position_++]);
    return value;
}

Result<i8> PacketBuffer::read_i8() {
    auto result = read_u8();
    if (!result) return result.error();
    return static_cast<i8>(result.value());
}

Result<u16> PacketBuffer::read_u16() {
    if (!ensure_available(2)) {
        return ErrorCode::ParseError;
    }
    u16 value = (static_cast<u16>(data_[position_]) << 8) |
                 static_cast<u16>(data_[position_ + 1]);
    position_ += 2;
    return value;
}

Result<i16> PacketBuffer::read_i16() {
    auto result = read_u16();
    if (!result) return result.error();
    return static_cast<i16>(result.value());
}

Result<u32> PacketBuffer::read_u32() {
    if (!ensure_available(4)) {
        return ErrorCode::ParseError;
    }
    u32 value = (static_cast<u32>(data_[position_]) << 24) |
                (static_cast<u32>(data_[position_ + 1]) << 16) |
                (static_cast<u32>(data_[position_ + 2]) << 8) |
                 static_cast<u32>(data_[position_ + 3]);
    position_ += 4;
    return value;
}

Result<i32> PacketBuffer::read_i32() {
    auto result = read_u32();
    if (!result) return result.error();
    return static_cast<i32>(result.value());
}

Result<u64> PacketBuffer::read_u64() {
    if (!ensure_available(8)) {
        return ErrorCode::ParseError;
    }
    u64 value = (static_cast<u64>(data_[position_]) << 56) |
                (static_cast<u64>(data_[position_ + 1]) << 48) |
                (static_cast<u64>(data_[position_ + 2]) << 40) |
                (static_cast<u64>(data_[position_ + 3]) << 32) |
                (static_cast<u64>(data_[position_ + 4]) << 24) |
                (static_cast<u64>(data_[position_ + 5]) << 16) |
                (static_cast<u64>(data_[position_ + 6]) << 8) |
                 static_cast<u64>(data_[position_ + 7]);
    position_ += 8;
    return value;
}

Result<i64> PacketBuffer::read_i64() {
    auto result = read_u64();
    if (!result) return result.error();
    return static_cast<i64>(result.value());
}

Result<f32> PacketBuffer::read_f32() {
    auto result = read_u32();
    if (!result) return result.error();
    f32 value;
    std::memcpy(&value, &result.value(), sizeof(f32));
    return value;
}

Result<f64> PacketBuffer::read_f64() {
    auto result = read_u64();
    if (!result) return result.error();
    f64 value;
    std::memcpy(&value, &result.value(), sizeof(f64));
    return value;
}

Result<bool> PacketBuffer::read_bool() {
    auto result = read_u8();
    if (!result) return result.error();
    return result.value() != 0;
}

Result<std::string> PacketBuffer::read_string(usize max_length) {
    auto length_result = read_i16();
    if (!length_result) {
        return length_result.error();
    }

    i16 length = length_result.value();
    if (length < 0) {
        return ErrorCode::ParseError;
    }

    if (static_cast<usize>(length) > max_length) {
        return ErrorCode::ParseError;
    }

    // Beta 1.7.3 uses UTF-16 encoding (2 bytes per character)
    usize bytes_needed = static_cast<usize>(length) * 2;
    if (!ensure_available(bytes_needed)) {
        return ErrorCode::ParseError;
    }

    std::string result;
    result.reserve(length);

    for (i16 i = 0; i < length; ++i) {
        u16 ch = (static_cast<u16>(data_[position_]) << 8) |
                  static_cast<u16>(data_[position_ + 1]);
        position_ += 2;

        // Simple conversion (ASCII only for now, proper UTF-16 would be more complex)
        if (ch < 128) {
            result.push_back(static_cast<char>(ch));
        } else {
            result.push_back('?'); // Replace non-ASCII with placeholder
        }
    }

    return result;
}

void PacketBuffer::write_u8(u8 value) {
    data_.push_back(static_cast<byte>(value));
}

void PacketBuffer::write_i8(i8 value) {
    write_u8(static_cast<u8>(value));
}

void PacketBuffer::write_u16(u16 value) {
    data_.push_back(static_cast<byte>(value >> 8));
    data_.push_back(static_cast<byte>(value & 0xFF));
}

void PacketBuffer::write_i16(i16 value) {
    write_u16(static_cast<u16>(value));
}

void PacketBuffer::write_u32(u32 value) {
    data_.push_back(static_cast<byte>(value >> 24));
    data_.push_back(static_cast<byte>((value >> 16) & 0xFF));
    data_.push_back(static_cast<byte>((value >> 8) & 0xFF));
    data_.push_back(static_cast<byte>(value & 0xFF));
}

void PacketBuffer::write_i32(i32 value) {
    write_u32(static_cast<u32>(value));
}

void PacketBuffer::write_u64(u64 value) {
    data_.push_back(static_cast<byte>(value >> 56));
    data_.push_back(static_cast<byte>((value >> 48) & 0xFF));
    data_.push_back(static_cast<byte>((value >> 40) & 0xFF));
    data_.push_back(static_cast<byte>((value >> 32) & 0xFF));
    data_.push_back(static_cast<byte>((value >> 24) & 0xFF));
    data_.push_back(static_cast<byte>((value >> 16) & 0xFF));
    data_.push_back(static_cast<byte>((value >> 8) & 0xFF));
    data_.push_back(static_cast<byte>(value & 0xFF));
}

void PacketBuffer::write_i64(i64 value) {
    write_u64(static_cast<u64>(value));
}

void PacketBuffer::write_f32(f32 value) {
    u32 bits;
    std::memcpy(&bits, &value, sizeof(f32));
    write_u32(bits);
}

void PacketBuffer::write_f64(f64 value) {
    u64 bits;
    std::memcpy(&bits, &value, sizeof(f64));
    write_u64(bits);
}

void PacketBuffer::write_bool(bool value) {
    write_u8(value ? 1 : 0);
}

void PacketBuffer::write_string(const std::string& str) {
    if (str.length() > 32767) {
        return; // String too long, should return error
    }

    write_i16(static_cast<i16>(str.length()));

    // Beta 1.7.3 uses UTF-16 encoding (2 bytes per character)
    for (char ch : str) {
        write_u16(static_cast<u16>(static_cast<unsigned char>(ch)));
    }
}

} // namespace mcserver
