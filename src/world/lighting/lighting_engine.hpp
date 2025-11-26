#pragma once

#include "util/types.hpp"
#include <queue>
#include <tuple>

namespace mcserver {

class ChunkManager;
class Chunk;

// Light levels (0-15, where 15 is brightest)
constexpr u8 MAX_LIGHT_LEVEL = 15;
constexpr u8 MIN_LIGHT_LEVEL = 0;

// Light node for propagation queue (x, y, z, light_level)
using LightNode = std::tuple<i32, i32, i32, u8>;

// LightingEngine handles all lighting calculations for the world
class LightingEngine {
public:
    explicit LightingEngine(ChunkManager* chunk_manager);

    // Initialize lighting for a newly generated chunk
    void initialize_chunk_lighting(Chunk* chunk, i32 chunk_x, i32 chunk_z);

    // Update lighting when a block is placed
    void update_light_on_block_place(i32 x, i32 y, i32 z, u8 block_id);

    // Update lighting when a block is broken
    void update_light_on_block_break(i32 x, i32 y, i32 z);

    // Recalculate sky light for a chunk (expensive, use sparingly)
    void recalculate_sky_light(Chunk* chunk, i32 chunk_x, i32 chunk_z);

    // Recalculate block light in an area (expensive, use sparingly)
    void recalculate_block_light_area(i32 center_x, i32 center_y, i32 center_z, i32 radius);

private:
    ChunkManager* chunk_manager_;

    // Sky light propagation
    void propagate_sky_light_down(i32 x, i32 z, i32 start_y, i32 chunk_x, i32 chunk_z);
    void propagate_sky_light_horizontal(i32 x, i32 y, i32 z);

    // Block light propagation
    void propagate_block_light_add(i32 x, i32 y, i32 z, u8 light_level);
    void propagate_block_light_remove(i32 x, i32 y, i32 z);

    // Light removal (for block breaking)
    void remove_sky_light(i32 x, i32 y, i32 z);
    void remove_block_light(i32 x, i32 y, i32 z);

    // Helper methods
    bool is_transparent(u8 block_id) const;
    bool is_light_source(u8 block_id) const;
    u8 get_block_light_emission(u8 block_id) const;

    // Get light values at world coordinates
    u8 get_sky_light_at(i32 x, i32 y, i32 z) const;
    u8 get_block_light_at(i32 x, i32 y, i32 z) const;
    u8 get_block_at(i32 x, i32 y, i32 z) const;

    // Set light values at world coordinates
    void set_sky_light_at(i32 x, i32 y, i32 z, u8 light);
    void set_block_light_at(i32 x, i32 y, i32 z, u8 light);

    // Convert world to chunk coordinates
    void world_to_chunk(i32 world_x, i32 world_z, i32& chunk_x, i32& chunk_z, i32& local_x, i32& local_z) const;

    // Get chunk at world coordinates
    Chunk* get_chunk_at(i32 world_x, i32 world_z) const;
};

} // namespace mcserver
