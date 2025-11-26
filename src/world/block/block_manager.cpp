#include "block_manager.hpp"
#include "world/chunk/chunk_manager.hpp"
#include "world/chunk/chunk.hpp"
#include "world/lighting/lighting_engine.hpp"
#include "entity/item/item_entity_manager.hpp"
#include "entity/inventory/item_stack.hpp"
#include "util/log/logger.hpp"
#include <random>

namespace mcserver {

BlockManager::BlockManager(ChunkManager* chunk_manager)
    : chunk_manager_(chunk_manager) {}

Result<void> BlockManager::break_block(i32 x, i8 y, i32 z) {
    // Validate coordinates (i8 is signed, so y can be negative)
    if (!can_break_block(x, y, z)) {
        return Result<void>(ErrorCode::PermissionDenied);
    }

    // Get chunk coordinates
    i32 chunk_x, chunk_z, local_x, local_z;
    world_to_chunk_coords(x, z, chunk_x, chunk_z, local_x, local_z);

    // Get or generate chunk
    Chunk* chunk = chunk_manager_->get_chunk(chunk_x, chunk_z);
    if (!chunk) {
        return Result<void>(ErrorCode::NotFound);
    }

    // Get the block type before breaking it
    u8 block_type = chunk->get_block(local_x, y, local_z);

    // Set block to air (0)
    chunk->set_block(local_x, y, local_z, 0);
    chunk->mark_dirty();  // Mark chunk as dirty for saving

    // Update lighting: when a block is removed, sky light should propagate down
    // Set this position to full sky light if it's exposed to sky
    i32 check_y = y + 1;
    bool exposed_to_sky = true;
    while (check_y < CHUNK_SIZE_Y) {
        u8 above_block = chunk->get_block(local_x, check_y, local_z);
        if (above_block != 0) {
            exposed_to_sky = false;
            break;
        }
        check_y++;
    }

    if (exposed_to_sky) {
        // Propagate sky light down from this position
        chunk->set_sky_light(local_x, y, local_z, 15);
        // Also update blocks below until we hit a solid block
        i32 propagate_y = y - 1;
        while (propagate_y >= 0) {
            u8 below_block = chunk->get_block(local_x, propagate_y, local_z);
            if (below_block != 0) {
                break;  // Hit solid block, stop propagating
            }
            chunk->set_sky_light(local_x, propagate_y, local_z, 15);
            propagate_y--;
        }
    }

    LOG_DEBUG_CAT("Block broken at (" + std::to_string(x) + ", " +
                  std::to_string(y) + ", " + std::to_string(z) + ")",
                  LogCategory::World);

    // Spawn item drop if block drops something
    if (item_entity_manager_ && block_type != 0) {  // Don't drop air
        // Use random generator for drop chances
        static std::random_device rd;
        static std::mt19937 gen(rd());
        std::uniform_real_distribution<f64> chance_dist(0.0, 1.0);
        std::uniform_real_distribution<f64> vel_dist(-0.1, 0.1);

        i16 drop_item = 0;
        i8 drop_count = 0;

        // Handle special drop chances
        if (block_type == 13) {  // Gravel: 10% chance for flint, 90% for gravel
            if (chance_dist(gen) < 0.10) {
                drop_item = 318;  // Flint
                drop_count = 1;
            } else {
                drop_item = 13;  // Gravel
                drop_count = 1;
            }
        } else if (block_type == 18) {  // Leaves: 5% chance for sapling
            if (chance_dist(gen) < 0.05) {
                drop_item = 6;  // Sapling
                drop_count = 1;
            }
            // Otherwise leaves drop nothing
        } else {
            // Normal drop logic
            drop_item = get_block_drop_item(block_type);
            drop_count = get_block_drop_count(block_type);
        }

        if (drop_item > 0 && drop_count > 0) {
            // Create item stack
            ItemStack dropped_item(drop_item, drop_count, 0);

            // Spawn at block center with small random velocity
            f64 item_x = static_cast<f64>(x) + 0.5;
            f64 item_y = static_cast<f64>(y) + 0.5;
            f64 item_z = static_cast<f64>(z) + 0.5;
            f64 vel_x = vel_dist(gen);
            f64 vel_y = 0.2;  // Small upward velocity
            f64 vel_z = vel_dist(gen);

            item_entity_manager_->spawn_item(dropped_item, item_x, item_y, item_z,
                                             vel_x, vel_y, vel_z);
        }
    }

    // Update lighting
    if (lighting_engine_) {
        lighting_engine_->update_light_on_block_break(x, y, z);
    }

    // Broadcast block change
    if (block_change_callback_) {
        block_change_callback_(x, y, z, 0, 0);
    }

    // Trigger chunk update to resend chunk with updated lighting
    if (chunk_update_callback_) {
        chunk_update_callback_(chunk_x, chunk_z);
    }

    return Result<void>();
}

Result<void> BlockManager::place_block(i32 x, i8 y, i32 z, u8 block_type, u8 metadata) {
    // Validate coordinates and block type
    if (block_type == 0) {
        // Can't place air
        return Result<void>(ErrorCode::InvalidArgument);
    }

    if (!can_place_block(x, y, z)) {
        return Result<void>(ErrorCode::PermissionDenied);
    }

    // Get chunk coordinates
    i32 chunk_x, chunk_z, local_x, local_z;
    world_to_chunk_coords(x, z, chunk_x, chunk_z, local_x, local_z);

    // Get or generate chunk
    Chunk* chunk = chunk_manager_->get_chunk(chunk_x, chunk_z);
    if (!chunk) {
        return Result<void>(ErrorCode::NotFound);
    }

    // Set the block
    chunk->set_block(local_x, y, local_z, block_type);
    chunk->mark_dirty();  // Mark chunk as dirty for saving

    LOG_DEBUG_CAT("Block placed at (" + std::to_string(x) + ", " +
                  std::to_string(y) + ", " + std::to_string(z) +
                  ") type: " + std::to_string(block_type),
                  LogCategory::World);

    // Update lighting
    if (lighting_engine_) {
        lighting_engine_->update_light_on_block_place(x, y, z, block_type);
    }

    // Broadcast block change
    if (block_change_callback_) {
        block_change_callback_(x, y, z, block_type, metadata);
    }

    return Result<void>();
}

Result<u8> BlockManager::get_block(i32 x, i8 y, i32 z) {
    // Get chunk coordinates
    i32 chunk_x, chunk_z, local_x, local_z;
    world_to_chunk_coords(x, z, chunk_x, chunk_z, local_x, local_z);

    // Get chunk
    Chunk* chunk = chunk_manager_->get_chunk(chunk_x, chunk_z);
    if (!chunk) {
        return Result<u8>(ErrorCode::NotFound);
    }

    u8 block = chunk->get_block(local_x, y, local_z);
    return Result<u8>(block);
}

bool BlockManager::can_place_block(i32 x, i8 y, i32 z) const {
    // Avoid unused parameter warnings
    (void)x;
    (void)z;
    (void)y;  // y is i8, so it's always in range -128 to 127

    // Basic validation
    // TODO: Add more validation:
    // - Check if position is already occupied by solid block
    // - Check if player is colliding with block position
    // - Check permissions/protection zones

    return true;
}

bool BlockManager::can_break_block(i32 x, i8 y, i32 z) const {
    // Avoid unused parameter warnings
    (void)x;
    (void)z;

    // Don't allow breaking bedrock at Y=0
    if (y == 0) {
        return false;
    }

    // TODO: Add more validation:
    // - Check permissions/protection zones
    // - Check if block is breakable

    return true;
}

void BlockManager::world_to_chunk_coords(i32 world_x, i32 world_z,
                                        i32& chunk_x, i32& chunk_z,
                                        i32& local_x, i32& local_z) const {
    // Chunk coordinates (divide by 16, floor division for negative numbers)
    chunk_x = world_x >> 4;  // Equivalent to world_x / 16
    chunk_z = world_z >> 4;

    // Handle negative coordinates properly
    if (world_x < 0 && (world_x & 15) != 0) {
        chunk_x--;
    }
    if (world_z < 0 && (world_z & 15) != 0) {
        chunk_z--;
    }

    // Local coordinates within chunk (0-15)
    local_x = world_x - (chunk_x * 16);
    local_z = world_z - (chunk_z * 16);

    // Ensure local coordinates are in valid range
    if (local_x < 0) local_x += 16;
    if (local_z < 0) local_z += 16;
}

i16 BlockManager::get_block_drop_item(u8 block_type) const {
    // Map block type to item ID
    // For most blocks, they drop themselves
    // Special cases need to be handled
    switch (block_type) {
        case 0:  // Air
            return 0;
        case 1:  // Stone
            return 4;  // Drops cobblestone
        case 2:  // Grass Block
            return 3;  // Drops dirt
        case 7:  // Bedrock - unbreakable, but if somehow broken, drop nothing
            return 0;
        case 8:  // Water (flowing)
        case 9:  // Water (still)
            return 0;  // Water doesn't drop
        case 10: // Lava (flowing)
        case 11: // Lava (still)
            return 0;  // Lava doesn't drop
        case 13: // Gravel
            return 13; // Drops gravel (10% flint chance handled in break_block)
        case 16: // Coal Ore
            return 263; // Drops coal item
        case 17: // Wood/Log
            return 17; // Drops wood log
        case 18: // Leaves
            return 0;  // Leaves decay (5% sapling chance handled in break_block)
        case 21: // Lapis Lazuli Ore
            return 351; // Drops lapis lazuli dye (4-8, we'll drop 6)
        case 51: // Fire
            return 0;  // Fire doesn't drop
        case 52: // Monster Spawner
            return 0;  // Spawner doesn't drop
        case 56: // Diamond Ore
            return 264; // Drops diamond
        case 60: // Farmland
            return 3;  // Drops dirt
        case 62: // Burning Furnace
            return 61; // Drops normal furnace
        case 63: // Sign Post
        case 68: // Wall Sign
            return 323; // Drops sign item
        case 73: // Redstone Ore
        case 74: // Glowing Redstone Ore
            return 331; // Drops redstone dust (4-5 pieces, but we'll drop 4)
        case 78: // Snow Layer
            return 332; // Drops snowball
        case 79: // Ice
            return 0;  // Ice doesn't drop (melts)
        case 82: // Clay Block
            return 337; // Drops clay balls (4)
        case 83: // Sugar Cane
            return 338; // Drops sugar cane item
        case 89: // Glowstone
            return 348; // Drops glowstone dust (2-4, we'll drop 3)
        case 93: // Redstone Repeater (Off)
        case 94: // Redstone Repeater (On)
            return 356; // Drops repeater item
        default:
            // Most blocks drop themselves
            return static_cast<i16>(block_type);
    }
}

i8 BlockManager::get_block_drop_count(u8 block_type) const {
    // Determine how many items to drop
    switch (block_type) {
        case 0:   // Air
        case 7:   // Bedrock
        case 8:   // Water
        case 9:   // Water
        case 10:  // Lava
        case 11:  // Lava
        case 18:  // Leaves (decay)
        case 51:  // Fire
        case 52:  // Monster Spawner
        case 79:  // Ice
            return 0;  // No drops
        case 21:  // Lapis Lazuli Ore
            return 6;  // 4-8 lapis lazuli (we'll drop 6)
        case 73:  // Redstone Ore
        case 74:  // Glowing Redstone Ore
            return 4;  // 4-5 redstone dust (we'll drop 4)
        case 82:  // Clay Block
            return 4;  // 4 clay balls
        case 89:  // Glowstone
            return 3;  // 2-4 glowstone dust (we'll drop 3)
        default:
            return 1;  // Most blocks drop 1
    }
}

} // namespace mcserver
