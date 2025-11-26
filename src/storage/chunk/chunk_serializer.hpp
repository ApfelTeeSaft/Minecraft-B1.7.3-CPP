#pragma once

#include "util/types.hpp"
#include "util/result.hpp"
#include "storage/nbt/nbt.hpp"
#include "world/chunk/chunk.hpp"
#include <memory>

namespace mcserver {

// Chunk serialization utilities for McRegion format
class ChunkSerializer {
public:
    // Serialize a chunk to NBT format
    // Returns NBT compound with "Level" tag containing chunk data
    static std::unique_ptr<NBTCompound> serialize(const Chunk& chunk, i64 world_time = 0);

    // Deserialize a chunk from NBT format
    // Expects NBT compound with "Level" tag containing chunk data
    static Result<std::unique_ptr<Chunk>> deserialize(const NBTCompound& nbt);

private:
    // Calculate heightmap for a chunk (highest non-air block at each XZ position)
    static void calculate_heightmap(const Chunk& chunk, std::vector<i8>& heightmap);
};

} // namespace mcserver
