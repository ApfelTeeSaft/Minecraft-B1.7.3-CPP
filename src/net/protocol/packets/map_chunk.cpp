#include "map_chunk.hpp"
#include <zlib.h>
#include <cstring>

namespace mcserver {

PacketMapChunk::PacketMapChunk(i32 x, i32 z)
    : x(x), y(0), z(z), size_x(15), size_y(127), size_z(15) {}

Result<void> PacketMapChunk::read(PacketBuffer& buffer) {
    auto x_result = buffer.read_i32();
    if (!x_result) return x_result.error();
    x = x_result.value();

    auto y_result = buffer.read_i16();
    if (!y_result) return y_result.error();
    y = y_result.value();

    auto z_result = buffer.read_i32();
    if (!z_result) return z_result.error();
    z = z_result.value();

    auto size_x_result = buffer.read_u8();
    if (!size_x_result) return size_x_result.error();
    size_x = size_x_result.value();

    auto size_y_result = buffer.read_u8();
    if (!size_y_result) return size_y_result.error();
    size_y = size_y_result.value();

    auto size_z_result = buffer.read_u8();
    if (!size_z_result) return size_z_result.error();
    size_z = size_z_result.value();

    auto compressed_size_result = buffer.read_i32();
    if (!compressed_size_result) return compressed_size_result.error();
    i32 compressed_size = compressed_size_result.value();

    if (compressed_size < 0 || compressed_size > 1024 * 1024) {
        return Result<void>(ErrorCode::ParseError);
    }

    // Read compressed data
    compressed_data.resize(static_cast<usize>(compressed_size));
    for (i32 i = 0; i < compressed_size; ++i) {
        auto byte_result = buffer.read_u8();
        if (!byte_result) return byte_result.error();
        compressed_data[i] = static_cast<byte>(byte_result.value());
    }

    return Result<void>();
}

Result<void> PacketMapChunk::write(PacketBuffer& buffer) const {
    buffer.write_i32(x);
    buffer.write_i16(y);
    buffer.write_i32(z);
    buffer.write_u8(size_x);
    buffer.write_u8(size_y);
    buffer.write_u8(size_z);
    buffer.write_i32(static_cast<i32>(compressed_data.size()));

    // Write compressed data
    for (byte b : compressed_data) {
        buffer.write_u8(static_cast<u8>(b));
    }

    return Result<void>();
}

usize PacketMapChunk::estimated_size() const {
    // 4 (x) + 2 (y) + 4 (z) + 1 (size_x) + 1 (size_y) + 1 (size_z) + 4 (compressed_size) + data
    return 17 + compressed_data.size();
}

void PacketMapChunk::set_chunk_data(const u8* blocks, const u8* metadata,
                                    const u8* block_light, const u8* sky_light) {
    // Prepare uncompressed data buffer
    std::vector<u8> uncompressed(TOTAL_DATA_SIZE);

    // Copy all chunk data into one buffer
    std::memcpy(uncompressed.data(), blocks, BLOCKS_SIZE);
    std::memcpy(uncompressed.data() + BLOCKS_SIZE, metadata, METADATA_SIZE);
    std::memcpy(uncompressed.data() + BLOCKS_SIZE + METADATA_SIZE, block_light, BLOCK_LIGHT_SIZE);
    std::memcpy(uncompressed.data() + BLOCKS_SIZE + METADATA_SIZE + BLOCK_LIGHT_SIZE,
                sky_light, SKY_LIGHT_SIZE);

    // Compress using zlib
    uLongf compressed_size = compressBound(TOTAL_DATA_SIZE);
    compressed_data.resize(compressed_size);

    int result = compress(
        reinterpret_cast<Bytef*>(compressed_data.data()),
        &compressed_size,
        uncompressed.data(),
        TOTAL_DATA_SIZE
    );

    if (result == Z_OK) {
        compressed_data.resize(compressed_size);
    } else {
        // Compression failed, store uncompressed (shouldn't happen)
        compressed_data.resize(TOTAL_DATA_SIZE);
        std::memcpy(compressed_data.data(), uncompressed.data(), TOTAL_DATA_SIZE);
    }

    // Cache the uncompressed data
    uncompressed_data_ = std::move(uncompressed);
    decompressed_ = true;
}

Result<void> PacketMapChunk::decompress_data() const {
    if (decompressed_) {
        return Result<void>();
    }

    uncompressed_data_.resize(TOTAL_DATA_SIZE);
    uLongf uncompressed_size = TOTAL_DATA_SIZE;

    int result = uncompress(
        uncompressed_data_.data(),
        &uncompressed_size,
        reinterpret_cast<const Bytef*>(compressed_data.data()),
        static_cast<uLong>(compressed_data.size())
    );

    if (result != Z_OK || uncompressed_size != TOTAL_DATA_SIZE) {
        return Result<void>(ErrorCode::ParseError);
    }

    decompressed_ = true;
    return Result<void>();
}

Result<const u8*> PacketMapChunk::get_blocks() {
    auto decompress_result = decompress_data();
    if (!decompress_result) {
        return Result<const u8*>(decompress_result.error());
    }
    return Result<const u8*>(uncompressed_data_.data());
}

Result<const u8*> PacketMapChunk::get_metadata() {
    auto decompress_result = decompress_data();
    if (!decompress_result) {
        return Result<const u8*>(decompress_result.error());
    }
    return Result<const u8*>(uncompressed_data_.data() + BLOCKS_SIZE);
}

Result<const u8*> PacketMapChunk::get_block_light() {
    auto decompress_result = decompress_data();
    if (!decompress_result) {
        return Result<const u8*>(decompress_result.error());
    }
    return Result<const u8*>(uncompressed_data_.data() + BLOCKS_SIZE + METADATA_SIZE);
}

Result<const u8*> PacketMapChunk::get_sky_light() {
    auto decompress_result = decompress_data();
    if (!decompress_result) {
        return Result<const u8*>(decompress_result.error());
    }
    return Result<const u8*>(uncompressed_data_.data() + BLOCKS_SIZE + METADATA_SIZE + BLOCK_LIGHT_SIZE);
}

} // namespace mcserver
