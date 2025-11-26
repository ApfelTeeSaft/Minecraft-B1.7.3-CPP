#pragma once

#include "net/protocol/packet.hpp"
#include <vector>

namespace mcserver {

// Packet 51: MapChunk
// Sends chunk terrain data to client
// Must be preceded by PreChunk packet with load=true
class PacketMapChunk : public Packet {
public:
    // Chunk dimensions for Beta 1.7.3
    static constexpr i32 CHUNK_WIDTH = 16;
    static constexpr i32 CHUNK_HEIGHT = 128;
    static constexpr i32 CHUNK_DEPTH = 16;

    // Uncompressed data sizes
    static constexpr usize BLOCKS_SIZE = CHUNK_WIDTH * CHUNK_HEIGHT * CHUNK_DEPTH;     // 32768
    static constexpr usize METADATA_SIZE = BLOCKS_SIZE / 2;                             // 16384
    static constexpr usize BLOCK_LIGHT_SIZE = BLOCKS_SIZE / 2;                          // 16384
    static constexpr usize SKY_LIGHT_SIZE = BLOCKS_SIZE / 2;                            // 16384
    static constexpr usize TOTAL_DATA_SIZE = BLOCKS_SIZE + METADATA_SIZE +
                                             BLOCK_LIGHT_SIZE + SKY_LIGHT_SIZE;         // 81920

    PacketMapChunk() = default;
    PacketMapChunk(i32 x, i32 z);

    PacketId get_id() const override { return PacketId::MapChunk; }
    Result<void> read(PacketBuffer& buffer) override;
    Result<void> write(PacketBuffer& buffer) const override;
    usize estimated_size() const override;

    // Set chunk data (will be compressed on write)
    // blocks: 32768 bytes - block IDs
    // metadata: 16384 bytes - 4 bits per block
    // block_light: 16384 bytes - 4 bits per block
    // sky_light: 16384 bytes - 4 bits per block
    void set_chunk_data(const u8* blocks, const u8* metadata,
                       const u8* block_light, const u8* sky_light);

    // Get uncompressed chunk data (decompresses on first call)
    Result<const u8*> get_blocks();
    Result<const u8*> get_metadata();
    Result<const u8*> get_block_light();
    Result<const u8*> get_sky_light();

    i32 x = 0;          // Block X coordinate (chunk_x * 16)
    i16 y = 0;          // Block Y coordinate (always 0 in Beta 1.7.3)
    i32 z = 0;          // Block Z coordinate (chunk_z * 16)
    u8 size_x = 15;     // Always 15 (16 - 1)
    u8 size_y = 127;    // Always 127 (128 - 1)
    u8 size_z = 15;     // Always 15 (16 - 1)

    std::vector<byte> compressed_data;

private:
    // Cached uncompressed data
    mutable std::vector<u8> uncompressed_data_;
    mutable bool decompressed_ = false;

    Result<void> decompress_data() const;
};

} // namespace mcserver
