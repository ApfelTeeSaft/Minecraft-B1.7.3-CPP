#pragma once

#include "util/types.hpp"
#include "util/result.hpp"
#include "storage/nbt/nbt.hpp"
#include <string>
#include <vector>
#include <fstream>
#include <memory>

namespace mcserver {

// McRegion file format handler
// Region files contain 32x32 chunks (1024 chunks total)
// Format:
//   - Header: 8192 bytes
//     - Chunk offsets: 4096 bytes (1024 x 4 bytes)
//       Each entry: 3 bytes sector offset + 1 byte sector count
//     - Chunk timestamps: 4096 bytes (1024 x 4 bytes)
//   - Data sectors: 4KB-aligned chunk data (compressed NBT)
class RegionFile {
public:
    static constexpr i32 REGION_SIZE = 32;  // 32x32 chunks per region
    static constexpr i32 SECTOR_SIZE = 4096;
    static constexpr i32 HEADER_SIZE = 8192;
    static constexpr i32 CHUNK_COUNT = 1024;  // 32x32

    // Compression types
    enum class CompressionType : u8 {
        GZip = 1,
        ZLib = 2
    };

    explicit RegionFile(const std::string& file_path);
    ~RegionFile();

    // Disable copy, allow move
    RegionFile(const RegionFile&) = delete;
    RegionFile& operator=(const RegionFile&) = delete;
    RegionFile(RegionFile&&) noexcept = default;
    RegionFile& operator=(RegionFile&&) noexcept = default;

    // Open the region file (creates if doesn't exist)
    Result<void> open();

    // Close the region file
    void close();

    // Check if a chunk is saved in this region
    bool has_chunk(i32 chunk_x, i32 chunk_z) const;

    // Read chunk data as NBT compound
    Result<std::unique_ptr<NBTCompound>> read_chunk(i32 chunk_x, i32 chunk_z);

    // Write chunk data from NBT compound
    Result<void> write_chunk(i32 chunk_x, i32 chunk_z, const NBTCompound& data);

    // Get the file path
    const std::string& get_file_path() const { return file_path_; }

private:
    // Convert chunk coordinates to index in offset/timestamp tables
    static i32 get_chunk_index(i32 chunk_x, i32 chunk_z);

    // Check if chunk coordinates are valid
    static bool is_valid_chunk(i32 chunk_x, i32 chunk_z);

    // Get offset entry for a chunk
    i32 get_offset(i32 chunk_x, i32 chunk_z) const;

    // Set offset entry for a chunk
    Result<void> set_offset(i32 chunk_x, i32 chunk_z, i32 offset);

    // Get timestamp for a chunk
    i32 get_timestamp(i32 chunk_x, i32 chunk_z) const;

    // Set timestamp for a chunk
    Result<void> set_timestamp(i32 chunk_x, i32 chunk_z, i32 timestamp);

    // Allocate sectors for chunk data
    Result<i32> allocate_sectors(i32 sector_count);

    // Free sectors used by a chunk
    void free_sectors(i32 sector_offset, i32 sector_count);

    // Read data from a sector
    Result<std::vector<u8>> read_sectors(i32 sector_offset, i32 sector_count);

    // Write data to sectors
    Result<void> write_sectors(i32 sector_offset, const std::vector<u8>& data);

    std::string file_path_;
    std::fstream file_;
    bool is_open_ = false;

    // Chunk offsets: 3 bytes offset + 1 byte count (packed into i32)
    std::vector<i32> offsets_;

    // Chunk timestamps
    std::vector<i32> timestamps_;

    // Free sector tracking
    std::vector<bool> sectors_free_;
};

} // namespace mcserver
