#include "chunk_streaming_manager.hpp"
#include "net/session/client_session.hpp"
#include "net/protocol/packets/pre_chunk.hpp"
#include "net/protocol/packets/map_chunk.hpp"
#include "world/chunk/chunk_manager.hpp"
#include "world/chunk/chunk.hpp"
#include "util/log/logger.hpp"
#include <cmath>

namespace mcserver {

ChunkStreamingManager::ChunkStreamingManager(ChunkManager* chunk_manager, i32 view_distance)
    : chunk_manager_(chunk_manager)
    , view_distance_(view_distance) {

    // Validate view distance
    if (view_distance_ < 3) {
        LOG_WARNING_CAT("View distance too small (" + std::to_string(view_distance_) +
                        "), setting to 3", LogCategory::Network);
        view_distance_ = 3;
    }
    if (view_distance_ > 15) {
        LOG_WARNING_CAT("View distance too large (" + std::to_string(view_distance_) +
                        "), setting to 15", LogCategory::Network);
        view_distance_ = 15;
    }

    LOG_INFO_CAT("Chunk streaming manager initialized with view distance: " +
                 std::to_string(view_distance_) + " (" +
                 std::to_string(view_distance_ * 16) + " blocks)",
                 LogCategory::Network);
}

ChunkStreamingManager::~ChunkStreamingManager() {
    player_states_.clear();
}

void ChunkStreamingManager::add_player(ClientSession* session, f64 x, f64 z) {
    if (!session) {
        return;
    }

    // Calculate chunk coordinates
    i32 chunk_x = static_cast<i32>(std::floor(x)) >> 4;
    i32 chunk_z = static_cast<i32>(std::floor(z)) >> 4;

    // Create player state
    PlayerChunkState state(session);
    state.last_update_x = x;
    state.last_update_z = z;

    LOG_INFO_CAT("Adding player to chunk streaming at chunk (" +
                 std::to_string(chunk_x) + ", " + std::to_string(chunk_z) + ")",
                 LogCategory::Network);

    // Send chunks in spiral pattern (Beta 1.7.3 algorithm)
    // Start with center chunk
    send_chunk(session, chunk_x, chunk_z);
    state.loaded_chunks.insert(ChunkCoord(chunk_x, chunk_z));

    // Spiral outward from center
    i32 offset_x = 0;
    i32 offset_z = 0;
    i32 dir_idx = 0;

    // For each ring distance from center (1 to view_distance*2)
    for (i32 ring = 1; ring <= view_distance_ * 2; ++ring) {
        // Each ring has 2 iterations per direction
        for (i32 iter = 0; iter < 2; ++iter) {
            const i32* dir = spiral_dirs_[dir_idx % 4];

            // Move 'ring' steps in current direction
            for (i32 step = 0; step < ring; ++step) {
                offset_x += dir[0];
                offset_z += dir[1];

                i32 cx = chunk_x + offset_x;
                i32 cz = chunk_z + offset_z;

                send_chunk(session, cx, cz);
                state.loaded_chunks.insert(ChunkCoord(cx, cz));
            }

            ++dir_idx;
        }
    }

    // Final strip to complete the square
    dir_idx %= 4;
    const i32* final_dir = spiral_dirs_[dir_idx];
    for (i32 step = 0; step < view_distance_ * 2; ++step) {
        offset_x += final_dir[0];
        offset_z += final_dir[1];

        i32 cx = chunk_x + offset_x;
        i32 cz = chunk_z + offset_z;

        send_chunk(session, cx, cz);
        state.loaded_chunks.insert(ChunkCoord(cx, cz));
    }

    // Store player state
    player_states_[session] = std::move(state);

    LOG_INFO_CAT("Sent " + std::to_string(player_states_[session].loaded_chunks.size()) +
                 " initial chunks to player", LogCategory::Network);
}

void ChunkStreamingManager::remove_player(ClientSession* session) {
    auto it = player_states_.find(session);
    if (it == player_states_.end()) {
        return;
    }

    LOG_INFO_CAT("Removing player from chunk streaming (" +
                 std::to_string(it->second.loaded_chunks.size()) + " chunks loaded)",
                 LogCategory::Network);

    // Unload all chunks for this player
    for (const auto& coord : it->second.loaded_chunks) {
        unload_chunk(session, coord.x, coord.z);
    }

    player_states_.erase(it);
}

void ChunkStreamingManager::update_player_chunks(ClientSession* session, f64 x, f64 z) {
    auto it = player_states_.find(session);
    if (it == player_states_.end()) {
        return;
    }

    PlayerChunkState& state = it->second;

    // Check if player has moved significantly (8+ blocks)
    f64 dx = state.last_update_x - x;
    f64 dz = state.last_update_z - z;
    f64 dist_sq = dx * dx + dz * dz;

    if (dist_sq < 64.0) {  // 8 * 8 = 64
        return;  // Player hasn't moved far enough
    }

    // Calculate current and previous chunk coordinates
    i32 curr_chunk_x = static_cast<i32>(std::floor(x)) >> 4;
    i32 curr_chunk_z = static_cast<i32>(std::floor(z)) >> 4;
    i32 prev_chunk_x = static_cast<i32>(std::floor(state.last_update_x)) >> 4;
    i32 prev_chunk_z = static_cast<i32>(std::floor(state.last_update_z)) >> 4;

    // Calculate movement delta in chunks
    i32 delta_x = curr_chunk_x - prev_chunk_x;
    i32 delta_z = curr_chunk_z - prev_chunk_z;

    // If player hasn't crossed chunk boundary, no update needed
    if (delta_x == 0 && delta_z == 0) {
        return;
    }

    LOG_DEBUG_CAT("Player moved from chunk (" + std::to_string(prev_chunk_x) + ", " +
                  std::to_string(prev_chunk_z) + ") to (" +
                  std::to_string(curr_chunk_x) + ", " + std::to_string(curr_chunk_z) + ")",
                  LogCategory::Network);

    // Track chunks to add and remove
    std::vector<ChunkCoord> chunks_to_add;
    std::vector<ChunkCoord> chunks_to_remove;

    // Iterate through all chunks in new view range
    for (i32 cx = curr_chunk_x - view_distance_; cx <= curr_chunk_x + view_distance_; ++cx) {
        for (i32 cz = curr_chunk_z - view_distance_; cz <= curr_chunk_z + view_distance_; ++cz) {
            ChunkCoord coord(cx, cz);

            // Check if chunk is in new range but not in old range
            if (!is_chunk_in_range(cx, cz, prev_chunk_x, prev_chunk_z)) {
                if (state.loaded_chunks.find(coord) == state.loaded_chunks.end()) {
                    chunks_to_add.push_back(coord);
                }
            }
        }
    }

    // Find chunks that are no longer in range
    for (const auto& coord : state.loaded_chunks) {
        if (!is_chunk_in_range(coord.x, coord.z, curr_chunk_x, curr_chunk_z)) {
            chunks_to_remove.push_back(coord);
        }
    }

    // Send new chunks
    for (const auto& coord : chunks_to_add) {
        send_chunk(session, coord.x, coord.z);
        state.loaded_chunks.insert(coord);
    }

    // Unload far chunks
    for (const auto& coord : chunks_to_remove) {
        unload_chunk(session, coord.x, coord.z);
        state.loaded_chunks.erase(coord);
    }

    if (!chunks_to_add.empty() || !chunks_to_remove.empty()) {
        LOG_DEBUG_CAT("Chunk update: +" + std::to_string(chunks_to_add.size()) +
                      " -" + std::to_string(chunks_to_remove.size()) +
                      " (total: " + std::to_string(state.loaded_chunks.size()) + ")",
                      LogCategory::Network);
    }

    // Update last position
    state.last_update_x = x;
    state.last_update_z = z;
}

void ChunkStreamingManager::set_view_distance(i32 distance) {
    if (distance < 3 || distance > 15) {
        LOG_WARNING_CAT("Invalid view distance: " + std::to_string(distance), LogCategory::Network);
        return;
    }

    view_distance_ = distance;
    LOG_INFO_CAT("View distance set to: " + std::to_string(distance) +
                 " (" + std::to_string(distance * 16) + " blocks)",
                 LogCategory::Network);
}

void ChunkStreamingManager::send_chunk(ClientSession* session, i32 chunk_x, i32 chunk_z) {
    if (!session || !chunk_manager_) {
        return;
    }

    // Send PreChunk packet to tell client to load this chunk
    PacketPreChunk pre_chunk(chunk_x, chunk_z, true);
    session->send_packet(pre_chunk);

    // Load/generate the chunk
    Chunk* chunk = chunk_manager_->get_chunk(chunk_x, chunk_z);
    if (chunk) {
        // Create MapChunk packet with compressed data
        PacketMapChunk map_chunk(chunk_x * 16, chunk_z * 16);
        map_chunk.set_chunk_data(
            chunk->get_blocks_data(),
            chunk->get_metadata_data(),
            chunk->get_block_light_data(),
            chunk->get_sky_light_data()
        );

        session->send_packet(map_chunk);

        // LOG_DEBUG_CAT("Sent chunk (" + std::to_string(chunk_x) + ", " +
        //               std::to_string(chunk_z) + ") to player",
        //               LogCategory::Network);
    } else {
        LOG_WARNING_CAT("Failed to load chunk (" + std::to_string(chunk_x) + ", " +
                        std::to_string(chunk_z) + ")", LogCategory::World);
    }
}

void ChunkStreamingManager::unload_chunk(ClientSession* session, i32 chunk_x, i32 chunk_z) {
    if (!session) {
        return;
    }

    // Send PreChunk with load=false to unload chunk
    PacketPreChunk pre_chunk(chunk_x, chunk_z, false);
    session->send_packet(pre_chunk);

    // LOG_DEBUG_CAT("Unloaded chunk (" + std::to_string(chunk_x) + ", " +
    //               std::to_string(chunk_z) + ") from player",
    //               LogCategory::Network);
}

bool ChunkStreamingManager::is_chunk_in_range(i32 chunk_x, i32 chunk_z,
                                               i32 center_x, i32 center_z) const {
    i32 dx = chunk_x - center_x;
    i32 dz = chunk_z - center_z;

    return (dx >= -view_distance_ && dx <= view_distance_ &&
            dz >= -view_distance_ && dz <= view_distance_);
}

} // namespace mcserver
