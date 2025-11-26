#pragma once

#include "util/types.hpp"
#include "util/result.hpp"
#include <functional>

namespace mcserver {

class ChunkManager;
class ClientSession;
class LightingEngine;
class ItemEntityManager;

// Callback for broadcasting block changes to nearby players
using BlockChangeCallback = std::function<void(i32 x, i8 y, i32 z, u8 block_type, u8 metadata)>;

// Callback for notifying when a chunk needs to be resent (e.g., for lighting updates)
using ChunkUpdateCallback = std::function<void(i32 chunk_x, i32 chunk_z)>;

// BlockManager handles block placement and breaking
class BlockManager {
public:
    explicit BlockManager(ChunkManager* chunk_manager);

    // Set callback for broadcasting block changes
    void set_block_change_callback(BlockChangeCallback callback) {
        block_change_callback_ = std::move(callback);
    }

    // Set callback for notifying when chunks need to be resent
    void set_chunk_update_callback(ChunkUpdateCallback callback) {
        chunk_update_callback_ = std::move(callback);
    }

    // Set lighting engine for automatic lighting updates
    void set_lighting_engine(LightingEngine* lighting_engine) {
        lighting_engine_ = lighting_engine;
    }

    // Set item entity manager for dropping items
    void set_item_entity_manager(ItemEntityManager* item_manager) {
        item_entity_manager_ = item_manager;
    }

    // Break a block at the given position
    Result<void> break_block(i32 x, i8 y, i32 z);

    // Place a block at the given position
    Result<void> place_block(i32 x, i8 y, i32 z, u8 block_type, u8 metadata = 0);

    // Get block at position
    Result<u8> get_block(i32 x, i8 y, i32 z);

    // Validate if a block can be placed at the position
    bool can_place_block(i32 x, i8 y, i32 z) const;

    // Validate if a block can be broken at the position
    bool can_break_block(i32 x, i8 y, i32 z) const;

private:
    ChunkManager* chunk_manager_;
    LightingEngine* lighting_engine_ = nullptr;
    ItemEntityManager* item_entity_manager_ = nullptr;
    BlockChangeCallback block_change_callback_;
    ChunkUpdateCallback chunk_update_callback_;

    // Helper to convert world coordinates to chunk coordinates
    void world_to_chunk_coords(i32 world_x, i32 world_z, i32& chunk_x, i32& chunk_z, i32& local_x, i32& local_z) const;

    // Helper to get the item that a block drops
    i16 get_block_drop_item(u8 block_type) const;

    // Helper to get the drop count for a block
    i8 get_block_drop_count(u8 block_type) const;
};

} // namespace mcserver
