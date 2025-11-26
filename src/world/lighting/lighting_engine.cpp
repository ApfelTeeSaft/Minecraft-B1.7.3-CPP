#include "lighting_engine.hpp"
#include "world/chunk/chunk_manager.hpp"
#include "world/chunk/chunk.hpp"
#include <algorithm>
#include <vector>

namespace mcserver {

LightingEngine::LightingEngine(ChunkManager* chunk_manager)
    : chunk_manager_(chunk_manager) {
}

void LightingEngine::initialize_chunk_lighting(Chunk* chunk, i32 chunk_x, i32 chunk_z) {
    if (!chunk) return;

    // Step 1: Initialize sky light from top down
    for (i32 x = 0; x < CHUNK_SIZE_X; ++x) {
        for (i32 z = 0; z < CHUNK_SIZE_Z; ++z) {
            // Start from the top with maximum sky light
            u8 current_light = MAX_LIGHT_LEVEL;

            for (i32 y = CHUNK_SIZE_Y - 1; y >= 0; --y) {
                u8 block = chunk->get_block(x, y, z);

                if (is_transparent(block)) {
                    // Transparent block - keep full sky light
                    chunk->set_sky_light(x, y, z, current_light);
                } else {
                    // Opaque block - no sky light below this point
                    chunk->set_sky_light(x, y, z, 0);
                    current_light = 0;
                }
            }
        }
    }

    // Step 2: Propagate sky light horizontally from edges
    for (i32 x = 0; x < CHUNK_SIZE_X; ++x) {
        for (i32 z = 0; z < CHUNK_SIZE_Z; ++z) {
            for (i32 y = 0; y < CHUNK_SIZE_Y; ++y) {
                u8 sky_light = chunk->get_sky_light(x, y, z);
                if (sky_light > 1) {
                    i32 world_x = chunk_x * CHUNK_SIZE_X + x;
                    i32 world_z = chunk_z * CHUNK_SIZE_Z + z;
                    propagate_sky_light_horizontal(world_x, y, world_z);
                }
            }
        }
    }

    // Step 3: Initialize block light from light sources
    for (i32 x = 0; x < CHUNK_SIZE_X; ++x) {
        for (i32 y = 0; y < CHUNK_SIZE_Y; ++y) {
            for (i32 z = 0; z < CHUNK_SIZE_Z; ++z) {
                u8 block = chunk->get_block(x, y, z);
                if (is_light_source(block)) {
                    u8 emission = get_block_light_emission(block);
                    chunk->set_block_light(x, y, z, emission);

                    // Propagate this light source
                    i32 world_x = chunk_x * CHUNK_SIZE_X + x;
                    i32 world_z = chunk_z * CHUNK_SIZE_Z + z;
                    propagate_block_light_add(world_x, y, world_z, emission);
                }
            }
        }
    }

    chunk->mark_dirty();
}

void LightingEngine::update_light_on_block_place(i32 x, i32 y, i32 z, u8 block_id) {
    // Remove existing light that was passing through this position
    remove_sky_light(x, y, z);

    // If the block is a light source, add its light
    if (is_light_source(block_id)) {
        u8 emission = get_block_light_emission(block_id);
        set_block_light_at(x, y, z, emission);
        propagate_block_light_add(x, y, z, emission);
    } else {
        // Not a light source, remove any block light
        remove_block_light(x, y, z);
    }

    // If opaque, block sky light above
    if (!is_transparent(block_id)) {
        set_sky_light_at(x, y, z, 0);

        // Remove sky light below
        for (i32 dy = y - 1; dy >= 0; --dy) {
            u8 below_block = get_block_at(x, dy, z);
            if (!is_transparent(below_block)) break;

            set_sky_light_at(x, dy, z, 0);
        }
    }
}

void LightingEngine::update_light_on_block_break(i32 x, i32 y, i32 z) {
    // Remove light from this position
    remove_block_light(x, y, z);
    remove_sky_light(x, y, z);

    // Check if sky light should propagate down
    u8 light_above = get_sky_light_at(x, y + 1, z);
    if (light_above == MAX_LIGHT_LEVEL) {
        // Full sky light above - propagate down
        i32 chunk_x, chunk_z, local_x, local_z;
        world_to_chunk(x, z, chunk_x, chunk_z, local_x, local_z);
        propagate_sky_light_down(x, z, y, chunk_x, chunk_z);
    } else if (light_above > 0) {
        // Partial sky light - propagate horizontally and down
        propagate_sky_light_horizontal(x, y, z);
    }

    // Check surrounding blocks for light sources
    static const i32 neighbors[6][3] = {
        {1, 0, 0}, {-1, 0, 0},
        {0, 1, 0}, {0, -1, 0},
        {0, 0, 1}, {0, 0, -1}
    };

    for (const auto& neighbor : neighbors) {
        i32 nx = x + neighbor[0];
        i32 ny = y + neighbor[1];
        i32 nz = z + neighbor[2];

        if (ny < 0 || ny >= CHUNK_SIZE_Y) continue;

        u8 neighbor_block_light = get_block_light_at(nx, ny, nz);
        if (neighbor_block_light > 1) {
            propagate_block_light_add(nx, ny, nz, neighbor_block_light);
        }

        u8 neighbor_sky_light = get_sky_light_at(nx, ny, nz);
        if (neighbor_sky_light > 1) {
            propagate_sky_light_horizontal(nx, ny, nz);
        }
    }
}

void LightingEngine::propagate_sky_light_down(i32 x, i32 z, i32 start_y, i32 chunk_x, i32 chunk_z) {
    (void)chunk_x;  // May be used for optimization
    (void)chunk_z;

    for (i32 y = start_y; y >= 0; --y) {
        u8 block = get_block_at(x, y, z);

        if (is_transparent(block)) {
            set_sky_light_at(x, y, z, MAX_LIGHT_LEVEL);
        } else {
            // Hit opaque block, stop
            break;
        }
    }
}

void LightingEngine::propagate_sky_light_horizontal(i32 x, i32 y, i32 z) {
    std::queue<LightNode> light_queue;
    u8 initial_light = get_sky_light_at(x, y, z);
    light_queue.push(std::make_tuple(x, y, z, initial_light));

    static const i32 neighbors[6][3] = {
        {1, 0, 0}, {-1, 0, 0},
        {0, 1, 0}, {0, -1, 0},
        {0, 0, 1}, {0, 0, -1}
    };

    while (!light_queue.empty()) {
        auto [cx, cy, cz, light] = light_queue.front();
        light_queue.pop();

        if (light <= 1) continue;  // Can't propagate further

        for (const auto& neighbor : neighbors) {
            i32 nx = cx + neighbor[0];
            i32 ny = cy + neighbor[1];
            i32 nz = cz + neighbor[2];

            if (ny < 0 || ny >= CHUNK_SIZE_Y) continue;

            u8 neighbor_block = get_block_at(nx, ny, nz);
            if (!is_transparent(neighbor_block)) continue;

            u8 new_light = light - 1;
            u8 current_light = get_sky_light_at(nx, ny, nz);

            if (new_light > current_light) {
                set_sky_light_at(nx, ny, nz, new_light);
                light_queue.push(std::make_tuple(nx, ny, nz, new_light));
            }
        }
    }
}

void LightingEngine::propagate_block_light_add(i32 x, i32 y, i32 z, u8 light_level) {
    std::queue<LightNode> light_queue;
    light_queue.push(std::make_tuple(x, y, z, light_level));

    static const i32 neighbors[6][3] = {
        {1, 0, 0}, {-1, 0, 0},
        {0, 1, 0}, {0, -1, 0},
        {0, 0, 1}, {0, 0, -1}
    };

    while (!light_queue.empty()) {
        auto [cx, cy, cz, light] = light_queue.front();
        light_queue.pop();

        if (light <= 1) continue;  // Can't propagate further

        for (const auto& neighbor : neighbors) {
            i32 nx = cx + neighbor[0];
            i32 ny = cy + neighbor[1];
            i32 nz = cz + neighbor[2];

            if (ny < 0 || ny >= CHUNK_SIZE_Y) continue;

            u8 neighbor_block = get_block_at(nx, ny, nz);
            if (!is_transparent(neighbor_block)) continue;

            u8 new_light = light - 1;
            u8 current_light = get_block_light_at(nx, ny, nz);

            if (new_light > current_light) {
                set_block_light_at(nx, ny, nz, new_light);
                light_queue.push(std::make_tuple(nx, ny, nz, new_light));
            }
        }
    }
}

void LightingEngine::propagate_block_light_remove(i32 x, i32 y, i32 z) {
    // Use flood fill to remove light
    std::queue<LightNode> removal_queue;
    std::queue<LightNode> relight_queue;

    u8 old_light = get_block_light_at(x, y, z);
    removal_queue.push(std::make_tuple(x, y, z, old_light));
    set_block_light_at(x, y, z, 0);

    static const i32 neighbors[6][3] = {
        {1, 0, 0}, {-1, 0, 0},
        {0, 1, 0}, {0, -1, 0},
        {0, 0, 1}, {0, 0, -1}
    };

    while (!removal_queue.empty()) {
        auto [cx, cy, cz, light] = removal_queue.front();
        removal_queue.pop();

        for (const auto& neighbor : neighbors) {
            i32 nx = cx + neighbor[0];
            i32 ny = cy + neighbor[1];
            i32 nz = cz + neighbor[2];

            if (ny < 0 || ny >= CHUNK_SIZE_Y) continue;

            u8 neighbor_light = get_block_light_at(nx, ny, nz);

            if (neighbor_light > 0 && neighbor_light < light) {
                // Remove this light
                set_block_light_at(nx, ny, nz, 0);
                removal_queue.push(std::make_tuple(nx, ny, nz, neighbor_light));
            } else if (neighbor_light >= light) {
                // This is a light source or stronger light - re-propagate later
                relight_queue.push(std::make_tuple(nx, ny, nz, neighbor_light));
            }
        }
    }

    // Re-propagate light from sources
    while (!relight_queue.empty()) {
        auto [cx, cy, cz, light] = relight_queue.front();
        relight_queue.pop();
        propagate_block_light_add(cx, cy, cz, light);
    }
}

void LightingEngine::remove_sky_light(i32 x, i32 y, i32 z) {
    // Similar to block light removal
    std::queue<LightNode> removal_queue;
    std::queue<LightNode> relight_queue;

    u8 old_light = get_sky_light_at(x, y, z);
    removal_queue.push(std::make_tuple(x, y, z, old_light));
    set_sky_light_at(x, y, z, 0);

    static const i32 neighbors[6][3] = {
        {1, 0, 0}, {-1, 0, 0},
        {0, 1, 0}, {0, -1, 0},
        {0, 0, 1}, {0, 0, -1}
    };

    while (!removal_queue.empty()) {
        auto [cx, cy, cz, light] = removal_queue.front();
        removal_queue.pop();

        for (const auto& neighbor : neighbors) {
            i32 nx = cx + neighbor[0];
            i32 ny = cy + neighbor[1];
            i32 nz = cz + neighbor[2];

            if (ny < 0 || ny >= CHUNK_SIZE_Y) continue;

            u8 neighbor_light = get_sky_light_at(nx, ny, nz);

            if (neighbor_light > 0 && neighbor_light < light) {
                set_sky_light_at(nx, ny, nz, 0);
                removal_queue.push(std::make_tuple(nx, ny, nz, neighbor_light));
            } else if (neighbor_light >= light) {
                relight_queue.push(std::make_tuple(nx, ny, nz, neighbor_light));
            }
        }
    }

    // Re-propagate sky light
    while (!relight_queue.empty()) {
        auto [cx, cy, cz, light] = relight_queue.front();
        relight_queue.pop();
        propagate_sky_light_horizontal(cx, cy, cz);
    }
}

void LightingEngine::remove_block_light(i32 x, i32 y, i32 z) {
    propagate_block_light_remove(x, y, z);
}

void LightingEngine::recalculate_sky_light(Chunk* chunk, i32 chunk_x, i32 chunk_z) {
    // Clear all sky light first
    for (i32 x = 0; x < CHUNK_SIZE_X; ++x) {
        for (i32 y = 0; y < CHUNK_SIZE_Y; ++y) {
            for (i32 z = 0; z < CHUNK_SIZE_Z; ++z) {
                chunk->set_sky_light(x, y, z, 0);
            }
        }
    }

    // Recalculate from top down
    initialize_chunk_lighting(chunk, chunk_x, chunk_z);
}

void LightingEngine::recalculate_block_light_area(i32 center_x, i32 center_y, i32 center_z, i32 radius) {
    // Clear block light in area
    for (i32 dx = -radius; dx <= radius; ++dx) {
        for (i32 dy = -radius; dy <= radius; ++dy) {
            for (i32 dz = -radius; dz <= radius; ++dz) {
                i32 x = center_x + dx;
                i32 y = center_y + dy;
                i32 z = center_z + dz;

                if (y < 0 || y >= CHUNK_SIZE_Y) continue;

                set_block_light_at(x, y, z, 0);
            }
        }
    }

    // Recalculate from light sources
    for (i32 dx = -radius; dx <= radius; ++dx) {
        for (i32 dy = -radius; dy <= radius; ++dy) {
            for (i32 dz = -radius; dz <= radius; ++dz) {
                i32 x = center_x + dx;
                i32 y = center_y + dy;
                i32 z = center_z + dz;

                if (y < 0 || y >= CHUNK_SIZE_Y) continue;

                u8 block = get_block_at(x, y, z);
                if (is_light_source(block)) {
                    u8 emission = get_block_light_emission(block);
                    set_block_light_at(x, y, z, emission);
                    propagate_block_light_add(x, y, z, emission);
                }
            }
        }
    }
}

bool LightingEngine::is_transparent(u8 block_id) const {
    // Beta 1.7.3 transparent blocks
    return block_id == 0 ||  // Air
           block_id == 6 ||  // Sapling
           block_id == 8 ||  // Water (flowing)
           block_id == 9 ||  // Water (still)
           block_id == 18 || // Leaves
           block_id == 20 || // Glass
           block_id == 37 || // Yellow flower
           block_id == 38 || // Red rose
           block_id == 39 || // Brown mushroom
           block_id == 40 || // Red mushroom
           block_id == 50 || // Torch
           block_id == 51 || // Fire
           block_id == 59 || // Wheat
           block_id == 63 || // Sign post
           block_id == 64 || // Wooden door
           block_id == 65 || // Ladder
           block_id == 66 || // Rail
           block_id == 68 || // Wall sign
           block_id == 71 || // Iron door
           block_id == 75 || // Redstone torch (off)
           block_id == 76 || // Redstone torch (on)
           block_id == 77 || // Stone button
           block_id == 78 || // Snow layer
           block_id == 83 || // Sugar cane
           block_id == 85;   // Fence
}

bool LightingEngine::is_light_source(u8 block_id) const {
    return get_block_light_emission(block_id) > 0;
}

u8 LightingEngine::get_block_light_emission(u8 block_id) const {
    // Beta 1.7.3 light-emitting blocks
    switch (block_id) {
        case 10: // Lava (flowing)
        case 11: // Lava (still)
            return 15;
        case 50: // Torch
            return 14;
        case 51: // Fire
            return 15;
        case 62: // Furnace (lit)
            return 13;
        case 74: // Redstone ore (glowing)
            return 9;
        case 76: // Redstone torch (on)
            return 7;
        case 89: // Glowstone
            return 15;
        case 91: // Jack-o-lantern
            return 15;
        default:
            return 0;
    }
}

u8 LightingEngine::get_sky_light_at(i32 x, i32 y, i32 z) const {
    if (y < 0 || y >= CHUNK_SIZE_Y) return 0;

    i32 chunk_x, chunk_z, local_x, local_z;
    world_to_chunk(x, z, chunk_x, chunk_z, local_x, local_z);

    Chunk* chunk = chunk_manager_->get_chunk_if_loaded(chunk_x, chunk_z);
    if (!chunk) return 0;

    return chunk->get_sky_light(local_x, y, local_z);
}

u8 LightingEngine::get_block_light_at(i32 x, i32 y, i32 z) const {
    if (y < 0 || y >= CHUNK_SIZE_Y) return 0;

    i32 chunk_x, chunk_z, local_x, local_z;
    world_to_chunk(x, z, chunk_x, chunk_z, local_x, local_z);

    Chunk* chunk = chunk_manager_->get_chunk_if_loaded(chunk_x, chunk_z);
    if (!chunk) return 0;

    return chunk->get_block_light(local_x, y, local_z);
}

u8 LightingEngine::get_block_at(i32 x, i32 y, i32 z) const {
    if (y < 0 || y >= CHUNK_SIZE_Y) return 0;

    i32 chunk_x, chunk_z, local_x, local_z;
    world_to_chunk(x, z, chunk_x, chunk_z, local_x, local_z);

    Chunk* chunk = chunk_manager_->get_chunk_if_loaded(chunk_x, chunk_z);
    if (!chunk) return 0;

    return chunk->get_block(local_x, y, local_z);
}

void LightingEngine::set_sky_light_at(i32 x, i32 y, i32 z, u8 light) {
    if (y < 0 || y >= CHUNK_SIZE_Y) return;

    i32 chunk_x, chunk_z, local_x, local_z;
    world_to_chunk(x, z, chunk_x, chunk_z, local_x, local_z);

    Chunk* chunk = chunk_manager_->get_chunk_if_loaded(chunk_x, chunk_z);
    if (!chunk) return;

    chunk->set_sky_light(local_x, y, local_z, light);
    chunk->mark_dirty();
}

void LightingEngine::set_block_light_at(i32 x, i32 y, i32 z, u8 light) {
    if (y < 0 || y >= CHUNK_SIZE_Y) return;

    i32 chunk_x, chunk_z, local_x, local_z;
    world_to_chunk(x, z, chunk_x, chunk_z, local_x, local_z);

    Chunk* chunk = chunk_manager_->get_chunk_if_loaded(chunk_x, chunk_z);
    if (!chunk) return;

    chunk->set_block_light(local_x, y, local_z, light);
    chunk->mark_dirty();
}

void LightingEngine::world_to_chunk(i32 world_x, i32 world_z, i32& chunk_x, i32& chunk_z, i32& local_x, i32& local_z) const {
    chunk_x = world_x >> 4;  // Divide by 16
    chunk_z = world_z >> 4;
    local_x = world_x & 15;  // Modulo 16
    local_z = world_z & 15;

    // Handle negative coordinates
    if (world_x < 0 && local_x != 0) {
        chunk_x--;
        local_x = 16 + local_x;
    }
    if (world_z < 0 && local_z != 0) {
        chunk_z--;
        local_z = 16 + local_z;
    }
}

Chunk* LightingEngine::get_chunk_at(i32 world_x, i32 world_z) const {
    i32 chunk_x, chunk_z, local_x, local_z;
    world_to_chunk(world_x, world_z, chunk_x, chunk_z, local_x, local_z);
    return chunk_manager_->get_chunk_if_loaded(chunk_x, chunk_z);
}

} // namespace mcserver
