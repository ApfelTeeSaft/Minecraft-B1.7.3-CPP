#pragma once

#include "chunk.hpp"
#include "world/generation/world_generator.hpp"
#include <memory>
#include <map>
#include <vector>

namespace mcserver {

class ChunkStorage;

class ChunkManager {
public:
    explicit ChunkManager(WorldGenerator* generator, ChunkStorage* storage = nullptr);

    // Get or generate a chunk
    Chunk* get_chunk(i32 chunk_x, i32 chunk_z);

    // Get chunk if it exists (don't generate)
    Chunk* get_chunk_if_loaded(i32 chunk_x, i32 chunk_z);

    // Load a chunk (from storage or generate if needed)
    Chunk* load_chunk(i32 chunk_x, i32 chunk_z);

    // Unload a chunk (saves if dirty)
    void unload_chunk(i32 chunk_x, i32 chunk_z);

    // Check if chunk is loaded
    bool is_chunk_loaded(i32 chunk_x, i32 chunk_z) const;

    // Get all loaded chunks
    std::vector<Chunk*> get_loaded_chunks();

    // Save all dirty chunks
    void save_all_dirty();

    // Save all loaded chunks (dirty or not)
    void save_all();

    // Update logic (placeholder for future use)
    void tick();

    // Get chunk count
    usize get_loaded_chunk_count() const { return chunks_.size(); }

    // Set chunk storage (can be set after construction)
    void set_storage(ChunkStorage* storage) { storage_ = storage; }

private:
    WorldGenerator* generator_;
    ChunkStorage* storage_;
    std::map<std::pair<i32, i32>, std::unique_ptr<Chunk>> chunks_;

    // Helper to create chunk key
    static std::pair<i32, i32> make_key(i32 x, i32 z) {
        return std::make_pair(x, z);
    }
};

} // namespace mcserver
