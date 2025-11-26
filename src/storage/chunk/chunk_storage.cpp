#include "storage/chunk/chunk_storage.hpp"
#include "storage/chunk/chunk_serializer.hpp"
#include <filesystem>
#include <sstream>
#include <iomanip>

namespace mcserver {

ChunkStorage::ChunkStorage(const std::string& world_path)
    : world_path_(world_path) {
    // Create region directory if it doesn't exist
    std::filesystem::path region_path = std::filesystem::path(world_path) / "region";
    std::filesystem::create_directories(region_path);
}

ChunkStorage::~ChunkStorage() {
    close_all();
}

void ChunkStorage::chunk_to_region(i32 chunk_x, i32 chunk_z, i32& region_x, i32& region_z) {
    // Region files contain 32x32 chunks
    // Use arithmetic right shift to handle negative coordinates
    region_x = chunk_x >> 5;  // Divide by 32
    region_z = chunk_z >> 5;
}

void ChunkStorage::chunk_to_local(i32 chunk_x, i32 chunk_z, i32& local_x, i32& local_z) {
    // Local coordinates within region (0-31)
    local_x = chunk_x & 31;  // Modulo 32
    local_z = chunk_z & 31;
}

std::string ChunkStorage::get_region_file_path(i32 region_x, i32 region_z) const {
    std::ostringstream oss;
    oss << world_path_ << "/region/r." << region_x << "." << region_z << ".mcr";
    return oss.str();
}

Result<RegionFile*> ChunkStorage::get_region_file(i32 chunk_x, i32 chunk_z) {
    i32 region_x, region_z;
    chunk_to_region(chunk_x, chunk_z, region_x, region_z);

    i64 key = pack_region_key(region_x, region_z);

    // Check if region file is already open
    auto it = region_files_.find(key);
    if (it != region_files_.end()) {
        return it->second.get();
    }

    // Create new region file
    std::string file_path = get_region_file_path(region_x, region_z);
    auto region_file = std::make_unique<RegionFile>(file_path);

    // Open the region file
    auto open_result = region_file->open();
    if (!open_result) {
        return open_result.error();
    }

    // Store in cache
    RegionFile* ptr = region_file.get();
    region_files_[key] = std::move(region_file);

    return ptr;
}

Result<void> ChunkStorage::save_chunk(const Chunk& chunk, i64 world_time) {
    // Get region file
    auto region_result = get_region_file(chunk.get_x(), chunk.get_z());
    if (!region_result) {
        return region_result.error();
    }
    RegionFile* region = region_result.value();

    // Serialize chunk to NBT
    auto nbt = ChunkSerializer::serialize(chunk, world_time);

    // Get Level compound from root
    NBTCompound* level = nbt->get_compound("Level");
    if (!level) {
        return ErrorCode::ParseError;
    }

    // Get local chunk coordinates within region
    i32 local_x, local_z;
    chunk_to_local(chunk.get_x(), chunk.get_z(), local_x, local_z);

    // Write chunk to region file
    return region->write_chunk(local_x, local_z, *level);
}

Result<std::unique_ptr<Chunk>> ChunkStorage::load_chunk(i32 chunk_x, i32 chunk_z) {
    // Get region file
    auto region_result = get_region_file(chunk_x, chunk_z);
    if (!region_result) {
        return region_result.error();
    }
    RegionFile* region = region_result.value();

    // Get local chunk coordinates within region
    i32 local_x, local_z;
    chunk_to_local(chunk_x, chunk_z, local_x, local_z);

    // Read chunk from region file
    auto nbt_result = region->read_chunk(local_x, local_z);
    if (!nbt_result) {
        return nbt_result.error();
    }

    // Create root compound with Level tag
    auto root = std::make_unique<NBTCompound>();
    root->set_tag("Level", std::move(nbt_result.value()));

    // Deserialize chunk from NBT
    return ChunkSerializer::deserialize(*root);
}

bool ChunkStorage::chunk_exists(i32 chunk_x, i32 chunk_z) {
    // Get region file
    auto region_result = get_region_file(chunk_x, chunk_z);
    if (!region_result) {
        return false;
    }
    RegionFile* region = region_result.value();

    // Get local chunk coordinates within region
    i32 local_x, local_z;
    chunk_to_local(chunk_x, chunk_z, local_x, local_z);

    // Check if chunk exists in region
    return region->has_chunk(local_x, local_z);
}

void ChunkStorage::close_all() {
    for (auto& [key, region] : region_files_) {
        region->close();
    }
    region_files_.clear();
}

} // namespace mcserver
