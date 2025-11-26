#pragma once

#include "util/types.hpp"
#include <unordered_map>
#include <unordered_set>
#include <vector>

namespace mcserver {

class ClientSession;
class ChunkManager;
class Chunk;

// Represents a chunk coordinate pair (x, z)
struct ChunkCoord {
    i32 x;
    i32 z;

    ChunkCoord() : x(0), z(0) {}
    ChunkCoord(i32 x_, i32 z_) : x(x_), z(z_) {}

    bool operator==(const ChunkCoord& other) const {
        return x == other.x && z == other.z;
    }

    // Convert to unique key for hash map
    i64 to_key() const {
        return (static_cast<i64>(x) + 0x7fffffff) |
               ((static_cast<i64>(z) + 0x7fffffff) << 32);
    }
};

// Hash function for ChunkCoord
struct ChunkCoordHash {
    std::size_t operator()(const ChunkCoord& coord) const {
        return static_cast<std::size_t>(coord.to_key());
    }
};

// Per-player chunk streaming state
struct PlayerChunkState {
    ClientSession* session;
    f64 last_update_x;  // Last position where chunks were updated
    f64 last_update_z;
    std::unordered_set<ChunkCoord, ChunkCoordHash> loaded_chunks;

    PlayerChunkState() : session(nullptr), last_update_x(0.0), last_update_z(0.0) {}
    PlayerChunkState(ClientSession* sess)
        : session(sess), last_update_x(0.0), last_update_z(0.0) {}
};

// Manages chunk streaming for all connected players
// Implements Minecraft Beta 1.7.3 PlayerManager logic
class ChunkStreamingManager {
public:
    explicit ChunkStreamingManager(ChunkManager* chunk_manager, i32 view_distance = 10);
    ~ChunkStreamingManager();

    // Add player and send initial chunks in spiral pattern
    void add_player(ClientSession* session, f64 x, f64 z);

    // Remove player and unload all their chunks
    void remove_player(ClientSession* session);

    // Update chunks for player if they've moved significantly (8+ blocks)
    // Should be called every tick for active players
    void update_player_chunks(ClientSession* session, f64 x, f64 z);

    // Set view distance (3-15 chunks, default 10)
    void set_view_distance(i32 distance);
    i32 get_view_distance() const { return view_distance_; }

private:
    ChunkManager* chunk_manager_;
    i32 view_distance_;  // In chunks (default 10 = 160 blocks radius)

    // Track player states
    std::unordered_map<ClientSession*, PlayerChunkState> player_states_;

    // Spiral pattern for loading chunks (closest first)
    // Directions: {1,0}, {0,1}, {-1,0}, {0,-1} (right, down, left, up)
    static constexpr i32 spiral_dirs_[4][2] = {
        {1, 0}, {0, 1}, {-1, 0}, {0, -1}
    };

    // Send a chunk to the client (PreChunk + MapChunk)
    void send_chunk(ClientSession* session, i32 chunk_x, i32 chunk_z);

    // Unload a chunk from the client (PreChunk with load=false)
    void unload_chunk(ClientSession* session, i32 chunk_x, i32 chunk_z);

    // Check if chunk is within view distance
    bool is_chunk_in_range(i32 chunk_x, i32 chunk_z, i32 center_x, i32 center_z) const;
};

} // namespace mcserver
