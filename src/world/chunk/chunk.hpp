#pragma once

#include "util/types.hpp"
#include <array>
#include <vector>

namespace mcserver {

// Beta 1.7.3 chunk dimensions
constexpr i32 CHUNK_SIZE_X = 16;
constexpr i32 CHUNK_SIZE_Y = 128;
constexpr i32 CHUNK_SIZE_Z = 16;
constexpr usize BLOCKS_PER_CHUNK = CHUNK_SIZE_X * CHUNK_SIZE_Y * CHUNK_SIZE_Z; // 32768

// Block IDs from Beta 1.7.3
enum class BlockId : u8 {
    Air = 0,
    Stone = 1,
    Grass = 2,
    Dirt = 3,
    Cobblestone = 4,
    WoodPlanks = 5,
    Sapling = 6,
    Bedrock = 7,
    WaterFlowing = 8,
    WaterStill = 9,
    LavaFlowing = 10,
    LavaStill = 11,
    Sand = 12,
    Gravel = 13,
    GoldOre = 14,
    IronOre = 15,
    CoalOre = 16,
    Wood = 17,
    Leaves = 18,
    Sponge = 19,
    Glass = 20,
    // ... more blocks, these are the common ones
};

class Chunk {
public:
    Chunk(i32 x, i32 z);

    i32 get_x() const { return x_; }
    i32 get_z() const { return z_; }

    // Block access (x, y, z in local chunk coordinates [0-15, 0-127, 0-15])
    u8 get_block(i32 x, i32 y, i32 z) const;
    void set_block(i32 x, i32 y, i32 z, u8 block_id);
    void set_block(i32 x, i32 y, i32 z, BlockId block_id);

    // Metadata access (4 bits per block)
    u8 get_metadata(i32 x, i32 y, i32 z) const;
    void set_metadata(i32 x, i32 y, i32 z, u8 metadata);

    // Lighting access (4 bits per block)
    u8 get_block_light(i32 x, i32 y, i32 z) const;
    void set_block_light(i32 x, i32 y, i32 z, u8 light_level);

    u8 get_sky_light(i32 x, i32 y, i32 z) const;
    void set_sky_light(i32 x, i32 y, i32 z, u8 light_level);

    // Direct data access for MapChunk packet
    const u8* get_blocks_data() const { return blocks_.data(); }
    const u8* get_metadata_data() const { return metadata_.data(); }
    const u8* get_block_light_data() const { return block_light_.data(); }
    const u8* get_sky_light_data() const { return sky_light_.data(); }

    // Mark chunk as modified (needs saving/resending)
    void mark_dirty() { dirty_ = true; }
    bool is_dirty() const { return dirty_; }
    void clear_dirty() { dirty_ = false; }

    // Check if chunk is generated
    bool is_generated() const { return generated_; }
    void mark_generated() { generated_ = true; }

private:
    i32 x_;  // Chunk X coordinate
    i32 z_;  // Chunk Z coordinate
    bool dirty_ = false;
    bool generated_ = false;

    // Block data: 16x128x16 = 32,768 bytes
    std::array<u8, BLOCKS_PER_CHUNK> blocks_{};

    // Metadata: 4 bits per block = 16,384 bytes
    std::array<u8, BLOCKS_PER_CHUNK / 2> metadata_{};

    // Block light: 4 bits per block = 16,384 bytes
    std::array<u8, BLOCKS_PER_CHUNK / 2> block_light_{};

    // Sky light: 4 bits per block = 16,384 bytes
    std::array<u8, BLOCKS_PER_CHUNK / 2> sky_light_{};

    // Convert local coordinates to array index
    static constexpr usize get_index(i32 x, i32 y, i32 z) {
        return static_cast<usize>(y + z * CHUNK_SIZE_Y + x * CHUNK_SIZE_Y * CHUNK_SIZE_Z);
    }

    // Get/set nibble (4-bit value) in packed array
    static u8 get_nibble(const u8* data, usize index);
    static void set_nibble(u8* data, usize index, u8 value);
};

} // namespace mcserver
