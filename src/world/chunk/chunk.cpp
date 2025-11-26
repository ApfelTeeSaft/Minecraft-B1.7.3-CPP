#include "chunk.hpp"
#include <cstring>
#include <algorithm>

namespace mcserver {

Chunk::Chunk(i32 x, i32 z) : x_(x), z_(z) {
    // Initialize all blocks to air
    std::fill(blocks_.begin(), blocks_.end(), static_cast<u8>(BlockId::Air));

    // Initialize metadata and lighting to 0
    std::fill(metadata_.begin(), metadata_.end(), static_cast<u8>(0));
    std::fill(block_light_.begin(), block_light_.end(), static_cast<u8>(0));

    // Initialize sky light to maximum (15 = 0xFF in nibble pairs)
    std::fill(sky_light_.begin(), sky_light_.end(), static_cast<u8>(0xFF));
}

u8 Chunk::get_block(i32 x, i32 y, i32 z) const {
    if (x < 0 || x >= CHUNK_SIZE_X || y < 0 || y >= CHUNK_SIZE_Y || z < 0 || z >= CHUNK_SIZE_Z) {
        return static_cast<u8>(BlockId::Air);
    }
    return blocks_[get_index(x, y, z)];
}

void Chunk::set_block(i32 x, i32 y, i32 z, u8 block_id) {
    if (x < 0 || x >= CHUNK_SIZE_X || y < 0 || y >= CHUNK_SIZE_Y || z < 0 || z >= CHUNK_SIZE_Z) {
        return;
    }
    blocks_[get_index(x, y, z)] = block_id;
    mark_dirty();
}

void Chunk::set_block(i32 x, i32 y, i32 z, BlockId block_id) {
    set_block(x, y, z, static_cast<u8>(block_id));
}

u8 Chunk::get_metadata(i32 x, i32 y, i32 z) const {
    if (x < 0 || x >= CHUNK_SIZE_X || y < 0 || y >= CHUNK_SIZE_Y || z < 0 || z >= CHUNK_SIZE_Z) {
        return 0;
    }
    return get_nibble(metadata_.data(), get_index(x, y, z));
}

void Chunk::set_metadata(i32 x, i32 y, i32 z, u8 metadata) {
    if (x < 0 || x >= CHUNK_SIZE_X || y < 0 || y >= CHUNK_SIZE_Y || z < 0 || z >= CHUNK_SIZE_Z) {
        return;
    }
    set_nibble(metadata_.data(), get_index(x, y, z), metadata & 0x0F);
    mark_dirty();
}

u8 Chunk::get_block_light(i32 x, i32 y, i32 z) const {
    if (x < 0 || x >= CHUNK_SIZE_X || y < 0 || y >= CHUNK_SIZE_Y || z < 0 || z >= CHUNK_SIZE_Z) {
        return 0;
    }
    return get_nibble(block_light_.data(), get_index(x, y, z));
}

void Chunk::set_block_light(i32 x, i32 y, i32 z, u8 light_level) {
    if (x < 0 || x >= CHUNK_SIZE_X || y < 0 || y >= CHUNK_SIZE_Y || z < 0 || z >= CHUNK_SIZE_Z) {
        return;
    }
    set_nibble(block_light_.data(), get_index(x, y, z), light_level & 0x0F);
    mark_dirty();
}

u8 Chunk::get_sky_light(i32 x, i32 y, i32 z) const {
    if (x < 0 || x >= CHUNK_SIZE_X || y < 0 || y >= CHUNK_SIZE_Y || z < 0 || z >= CHUNK_SIZE_Z) {
        return 15;
    }
    return get_nibble(sky_light_.data(), get_index(x, y, z));
}

void Chunk::set_sky_light(i32 x, i32 y, i32 z, u8 light_level) {
    if (x < 0 || x >= CHUNK_SIZE_X || y < 0 || y >= CHUNK_SIZE_Y || z < 0 || z >= CHUNK_SIZE_Z) {
        return;
    }
    set_nibble(sky_light_.data(), get_index(x, y, z), light_level & 0x0F);
    mark_dirty();
}

u8 Chunk::get_nibble(const u8* data, usize index) {
    usize byte_index = index / 2;
    bool high_nibble = (index % 2) == 1;

    if (high_nibble) {
        return (data[byte_index] >> 4) & 0x0F;
    } else {
        return data[byte_index] & 0x0F;
    }
}

void Chunk::set_nibble(u8* data, usize index, u8 value) {
    usize byte_index = index / 2;
    bool high_nibble = (index % 2) == 1;

    if (high_nibble) {
        data[byte_index] = (data[byte_index] & 0x0F) | ((value & 0x0F) << 4);
    } else {
        data[byte_index] = (data[byte_index] & 0xF0) | (value & 0x0F);
    }
}

} // namespace mcserver
