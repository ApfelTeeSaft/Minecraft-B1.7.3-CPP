#include "storage/chunk/chunk_serializer.hpp"
#include <cstring>
#include <algorithm>

namespace mcserver {

std::unique_ptr<NBTCompound> ChunkSerializer::serialize(const Chunk& chunk, i64 world_time) {
    auto root = std::make_unique<NBTCompound>();
    auto level = std::make_unique<NBTCompound>();

    // Chunk position
    level->set_int("xPos", chunk.get_x());
    level->set_int("zPos", chunk.get_z());

    // Last update time
    level->set_long("LastUpdate", world_time);

    // Blocks data (32768 bytes)
    const u8* blocks_data = chunk.get_blocks_data();
    std::vector<i8> blocks(BLOCKS_PER_CHUNK);
    std::memcpy(blocks.data(), blocks_data, BLOCKS_PER_CHUNK);
    level->set_byte_array("Blocks", std::move(blocks));

    // Metadata/Data (16384 bytes - 4 bits per block)
    const u8* metadata_data = chunk.get_metadata_data();
    std::vector<i8> metadata(BLOCKS_PER_CHUNK / 2);
    std::memcpy(metadata.data(), metadata_data, BLOCKS_PER_CHUNK / 2);
    level->set_byte_array("Data", std::move(metadata));

    // Sky light (16384 bytes - 4 bits per block)
    const u8* sky_light_data = chunk.get_sky_light_data();
    std::vector<i8> sky_light(BLOCKS_PER_CHUNK / 2);
    std::memcpy(sky_light.data(), sky_light_data, BLOCKS_PER_CHUNK / 2);
    level->set_byte_array("SkyLight", std::move(sky_light));

    // Block light (16384 bytes - 4 bits per block)
    const u8* block_light_data = chunk.get_block_light_data();
    std::vector<i8> block_light(BLOCKS_PER_CHUNK / 2);
    std::memcpy(block_light.data(), block_light_data, BLOCKS_PER_CHUNK / 2);
    level->set_byte_array("BlockLight", std::move(block_light));

    // Height map (256 bytes - 16x16, one byte per XZ column)
    std::vector<i8> heightmap(256);
    calculate_heightmap(chunk, heightmap);
    level->set_byte_array("HeightMap", std::move(heightmap));

    // Terrain populated flag
    level->set_bool("TerrainPopulated", chunk.is_generated());

    // Entities list (empty for now - will be added when entity system is integrated)
    auto entities_list = std::make_unique<NBTList>(NBTType::Compound);
    level->set_tag("Entities", std::move(entities_list));

    // Tile entities list (empty for now - will be added when tile entities are implemented)
    auto tile_entities_list = std::make_unique<NBTList>(NBTType::Compound);
    level->set_tag("TileEntities", std::move(tile_entities_list));

    // Wrap level in root compound
    root->set_tag("Level", std::move(level));

    return root;
}

Result<std::unique_ptr<Chunk>> ChunkSerializer::deserialize(const NBTCompound& nbt) {
    // Get Level compound
    NBTCompound* level = nbt.get_compound("Level");
    if (!level) {
        return ErrorCode::ParseError;
    }

    // Read chunk position
    auto x_result = level->get_int("xPos");
    auto z_result = level->get_int("zPos");
    if (!x_result || !z_result) {
        return ErrorCode::ParseError;
    }

    i32 chunk_x = x_result.value();
    i32 chunk_z = z_result.value();

    // Create chunk
    auto chunk = std::make_unique<Chunk>(chunk_x, chunk_z);

    // Read blocks data
    auto blocks_result = level->get_byte_array("Blocks");
    if (!blocks_result) {
        return ErrorCode::ParseError;
    }
    const auto& blocks = blocks_result.value();
    if (blocks.size() != BLOCKS_PER_CHUNK) {
        return ErrorCode::ParseError;
    }

    // Copy blocks data
    for (usize i = 0; i < BLOCKS_PER_CHUNK; ++i) {
        i32 x = static_cast<i32>((i / (CHUNK_SIZE_Y * CHUNK_SIZE_Z)) % CHUNK_SIZE_X);
        i32 y = static_cast<i32>(i % CHUNK_SIZE_Y);
        i32 z = static_cast<i32>((i / CHUNK_SIZE_Y) % CHUNK_SIZE_Z);
        chunk->set_block(x, y, z, static_cast<u8>(blocks[i]));
    }

    // Read metadata
    auto metadata_result = level->get_byte_array("Data");
    if (metadata_result && metadata_result.value().size() == BLOCKS_PER_CHUNK / 2) {
        const auto& metadata = metadata_result.value();
        for (usize i = 0; i < BLOCKS_PER_CHUNK; ++i) {
            i32 x = static_cast<i32>((i / (CHUNK_SIZE_Y * CHUNK_SIZE_Z)) % CHUNK_SIZE_X);
            i32 y = static_cast<i32>(i % CHUNK_SIZE_Y);
            i32 z = static_cast<i32>((i / CHUNK_SIZE_Y) % CHUNK_SIZE_Z);

            u8 packed_value = static_cast<u8>(metadata[i / 2]);
            u8 nibble_value = (i % 2 == 0) ? (packed_value & 0x0F) : ((packed_value >> 4) & 0x0F);
            chunk->set_metadata(x, y, z, nibble_value);
        }
    }

    // Read sky light
    auto sky_light_result = level->get_byte_array("SkyLight");
    if (sky_light_result && sky_light_result.value().size() == BLOCKS_PER_CHUNK / 2) {
        const auto& sky_light = sky_light_result.value();
        for (usize i = 0; i < BLOCKS_PER_CHUNK; ++i) {
            i32 x = static_cast<i32>((i / (CHUNK_SIZE_Y * CHUNK_SIZE_Z)) % CHUNK_SIZE_X);
            i32 y = static_cast<i32>(i % CHUNK_SIZE_Y);
            i32 z = static_cast<i32>((i / CHUNK_SIZE_Y) % CHUNK_SIZE_Z);

            u8 packed_value = static_cast<u8>(sky_light[i / 2]);
            u8 nibble_value = (i % 2 == 0) ? (packed_value & 0x0F) : ((packed_value >> 4) & 0x0F);
            chunk->set_sky_light(x, y, z, nibble_value);
        }
    }

    // Read block light
    auto block_light_result = level->get_byte_array("BlockLight");
    if (block_light_result && block_light_result.value().size() == BLOCKS_PER_CHUNK / 2) {
        const auto& block_light = block_light_result.value();
        for (usize i = 0; i < BLOCKS_PER_CHUNK; ++i) {
            i32 x = static_cast<i32>((i / (CHUNK_SIZE_Y * CHUNK_SIZE_Z)) % CHUNK_SIZE_X);
            i32 y = static_cast<i32>(i % CHUNK_SIZE_Y);
            i32 z = static_cast<i32>((i / CHUNK_SIZE_Y) % CHUNK_SIZE_Z);

            u8 packed_value = static_cast<u8>(block_light[i / 2]);
            u8 nibble_value = (i % 2 == 0) ? (packed_value & 0x0F) : ((packed_value >> 4) & 0x0F);
            chunk->set_block_light(x, y, z, nibble_value);
        }
    }

    // Check terrain populated flag
    auto terrain_populated = level->get_bool("TerrainPopulated");
    if (terrain_populated && terrain_populated.value()) {
        chunk->mark_generated();
    }

    // Note: Entities and TileEntities are skipped for now
    // They will be added when those systems are fully implemented

    return chunk;
}

void ChunkSerializer::calculate_heightmap(const Chunk& chunk, std::vector<i8>& heightmap) {
    heightmap.resize(256);

    for (i32 x = 0; x < CHUNK_SIZE_X; ++x) {
        for (i32 z = 0; z < CHUNK_SIZE_Z; ++z) {
            i32 height = 0;

            // Find highest non-air block
            for (i32 y = CHUNK_SIZE_Y - 1; y >= 0; --y) {
                if (chunk.get_block(x, y, z) != 0) {
                    height = y + 1;  // Height is one above the block
                    break;
                }
            }

            heightmap[x + z * CHUNK_SIZE_X] = static_cast<i8>(height);
        }
    }
}

} // namespace mcserver
