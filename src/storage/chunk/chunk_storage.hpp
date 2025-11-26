#pragma once

#include "util/types.hpp"
#include "util/result.hpp"
#include "world/chunk/chunk.hpp"
#include "storage/region/region_file.hpp"
#include <string>
#include <memory>
#include <unordered_map>

namespace mcserver {

// High-level chunk storage manager for McRegion format
// Manages region files and provides save/load interface
class ChunkStorage {
public:
    explicit ChunkStorage(const std::string& world_path);
    ~ChunkStorage();

    // Save a chunk to disk
    Result<void> save_chunk(const Chunk& chunk, i64 world_time = 0);

    // Load a chunk from disk
    Result<std::unique_ptr<Chunk>> load_chunk(i32 chunk_x, i32 chunk_z);

    // Check if a chunk exists on disk
    bool chunk_exists(i32 chunk_x, i32 chunk_z);

    // Close all open region files
    void close_all();

private:
    // Get or create region file for chunk coordinates
    Result<RegionFile*> get_region_file(i32 chunk_x, i32 chunk_z);

    // Convert chunk coordinates to region coordinates
    static void chunk_to_region(i32 chunk_x, i32 chunk_z, i32& region_x, i32& region_z);

    // Convert chunk coordinates to local region coordinates (0-31)
    static void chunk_to_local(i32 chunk_x, i32 chunk_z, i32& local_x, i32& local_z);

    // Get region file path for region coordinates
    std::string get_region_file_path(i32 region_x, i32 region_z) const;

    std::string world_path_;
    std::unordered_map<i64, std::unique_ptr<RegionFile>> region_files_;

    // Pack region coordinates into a single i64 key
    static i64 pack_region_key(i32 region_x, i32 region_z) {
        return (static_cast<i64>(region_x) << 32) | (static_cast<i64>(region_z) & 0xFFFFFFFF);
    }
};

} // namespace mcserver
