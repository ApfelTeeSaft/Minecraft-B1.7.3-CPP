#include "storage/region/region_file.hpp"
#include "storage/nbt/nbt_io.hpp"
#include <cstring>
#include <ctime>
#include <algorithm>

namespace mcserver {

// Helper functions for big-endian I/O
static void write_i32_be(std::fstream& file, i32 value) {
    u8 bytes[4] = {
        static_cast<u8>((value >> 24) & 0xFF),
        static_cast<u8>((value >> 16) & 0xFF),
        static_cast<u8>((value >> 8) & 0xFF),
        static_cast<u8>(value & 0xFF)
    };
    file.write(reinterpret_cast<const char*>(bytes), 4);
}

static i32 read_i32_be(std::fstream& file) {
    u8 bytes[4];
    file.read(reinterpret_cast<char*>(bytes), 4);
    return (bytes[0] << 24) | (bytes[1] << 16) | (bytes[2] << 8) | bytes[3];
}

RegionFile::RegionFile(const std::string& file_path)
    : file_path_(file_path),
      offsets_(CHUNK_COUNT, 0),
      timestamps_(CHUNK_COUNT, 0) {
}

RegionFile::~RegionFile() {
    close();
}

Result<void> RegionFile::open() {
    if (is_open_) {
        return Result<void>();
    }

    // Open file in read/write binary mode
    file_.open(file_path_, std::ios::in | std::ios::out | std::ios::binary);

    bool file_exists = file_.is_open();

    if (!file_exists) {
        // File doesn't exist, create it
        file_.open(file_path_, std::ios::out | std::ios::binary);
        if (!file_.is_open()) {
            return ErrorCode::IOError;
        }
        file_.close();

        // Reopen in read/write mode
        file_.open(file_path_, std::ios::in | std::ios::out | std::ios::binary);
        if (!file_.is_open()) {
            return ErrorCode::IOError;
        }
    }

    // Get file size
    file_.seekg(0, std::ios::end);
    std::streampos file_size = file_.tellg();
    file_.seekg(0, std::ios::beg);

    // Initialize header if file is new or too small
    if (file_size < HEADER_SIZE) {
        // Write empty header
        for (i32 i = 0; i < CHUNK_COUNT; ++i) {
            write_i32_be(file_, 0);  // offset
        }
        for (i32 i = 0; i < CHUNK_COUNT; ++i) {
            write_i32_be(file_, 0);  // timestamp
        }

        // Initialize sectors as free (first 2 sectors are header)
        i32 sector_count = 2;
        sectors_free_.resize(sector_count, false);
        sectors_free_[0] = false;  // Header sector 1
        sectors_free_[1] = false;  // Header sector 2
    } else {
        // Read existing header
        file_.seekg(0, std::ios::beg);

        // Read offsets
        for (i32 i = 0; i < CHUNK_COUNT; ++i) {
            offsets_[i] = read_i32_be(file_);
        }

        // Read timestamps
        for (i32 i = 0; i < CHUNK_COUNT; ++i) {
            timestamps_[i] = read_i32_be(file_);
        }

        // Calculate number of sectors
        i32 sector_count = static_cast<i32>(file_size) / SECTOR_SIZE;
        sectors_free_.resize(sector_count, true);

        // Mark header sectors as used
        sectors_free_[0] = false;
        sectors_free_[1] = false;

        // Mark sectors used by chunks
        for (i32 i = 0; i < CHUNK_COUNT; ++i) {
            i32 offset = offsets_[i];
            if (offset != 0) {
                i32 sector_offset = offset >> 8;
                i32 sector_count_chunk = offset & 0xFF;

                // Validate sector range
                if (sector_offset + sector_count_chunk <= sector_count) {
                    for (i32 j = 0; j < sector_count_chunk; ++j) {
                        if (sector_offset + j < sector_count) {
                            sectors_free_[sector_offset + j] = false;
                        }
                    }
                }
            }
        }
    }

    is_open_ = true;
    return Result<void>();
}

void RegionFile::close() {
    if (file_.is_open()) {
        file_.close();
    }
    is_open_ = false;
}

i32 RegionFile::get_chunk_index(i32 chunk_x, i32 chunk_z) {
    return (chunk_x & 31) + (chunk_z & 31) * 32;
}

bool RegionFile::is_valid_chunk(i32 chunk_x, i32 chunk_z) {
    return chunk_x >= 0 && chunk_x < REGION_SIZE &&
           chunk_z >= 0 && chunk_z < REGION_SIZE;
}

i32 RegionFile::get_offset(i32 chunk_x, i32 chunk_z) const {
    return offsets_[get_chunk_index(chunk_x, chunk_z)];
}

Result<void> RegionFile::set_offset(i32 chunk_x, i32 chunk_z, i32 offset) {
    if (!is_open_) {
        return ErrorCode::IOError;
    }

    i32 index = get_chunk_index(chunk_x, chunk_z);
    offsets_[index] = offset;

    // Write to file
    file_.seekp(index * 4);
    write_i32_be(file_, offset);
    file_.flush();

    return Result<void>();
}

i32 RegionFile::get_timestamp(i32 chunk_x, i32 chunk_z) const {
    return timestamps_[get_chunk_index(chunk_x, chunk_z)];
}

Result<void> RegionFile::set_timestamp(i32 chunk_x, i32 chunk_z, i32 timestamp) {
    if (!is_open_) {
        return ErrorCode::IOError;
    }

    i32 index = get_chunk_index(chunk_x, chunk_z);
    timestamps_[index] = timestamp;

    // Write to file
    file_.seekp(SECTOR_SIZE + index * 4);
    write_i32_be(file_, timestamp);
    file_.flush();

    return Result<void>();
}

bool RegionFile::has_chunk(i32 chunk_x, i32 chunk_z) const {
    if (!is_valid_chunk(chunk_x, chunk_z)) {
        return false;
    }
    return get_offset(chunk_x, chunk_z) != 0;
}

Result<std::unique_ptr<NBTCompound>> RegionFile::read_chunk(i32 chunk_x, i32 chunk_z) {
    if (!is_open_) {
        return ErrorCode::IOError;
    }

    if (!is_valid_chunk(chunk_x, chunk_z)) {
        return ErrorCode::InvalidArgument;
    }

    i32 offset = get_offset(chunk_x, chunk_z);
    if (offset == 0) {
        return ErrorCode::NotFound;
    }

    i32 sector_offset = offset >> 8;
    i32 sector_count = offset & 0xFF;

    if (sector_offset + sector_count > static_cast<i32>(sectors_free_.size())) {
        return ErrorCode::InvalidArgument;
    }

    // Seek to sector
    file_.seekg(sector_offset * SECTOR_SIZE);

    // Read chunk length (4 bytes)
    i32 length = read_i32_be(file_);
    if (length <= 0 || length > sector_count * SECTOR_SIZE) {
        return ErrorCode::InvalidArgument;
    }

    // Read compression type (1 byte)
    u8 compression_type;
    file_.read(reinterpret_cast<char*>(&compression_type), 1);

    // Read compressed data
    std::vector<u8> compressed_data(length - 1);
    file_.read(reinterpret_cast<char*>(compressed_data.data()), length - 1);

    if (!file_) {
        return ErrorCode::IOError;
    }

    // Decompress data
    Result<std::vector<u8>> decompressed_result = [&]() -> Result<std::vector<u8>> {
        if (compression_type == static_cast<u8>(CompressionType::ZLib)) {
            return nbt_compression::decompress_zlib(
                compressed_data.data(), compressed_data.size());
        } else if (compression_type == static_cast<u8>(CompressionType::GZip)) {
            return nbt_compression::decompress_gzip(
                compressed_data.data(), compressed_data.size());
        } else {
            return ErrorCode::InvalidArgument;
        }
    }();

    if (!decompressed_result) {
        return decompressed_result.error();
    }

    // Parse NBT data
    NBTReader reader(decompressed_result.value());
    return reader.read_compound();
}

Result<void> RegionFile::write_chunk(i32 chunk_x, i32 chunk_z, const NBTCompound& data) {
    if (!is_open_) {
        return ErrorCode::IOError;
    }

    if (!is_valid_chunk(chunk_x, chunk_z)) {
        return ErrorCode::InvalidArgument;
    }

    // Serialize NBT data
    NBTWriter writer;
    writer.write_compound("", data);
    std::vector<u8> nbt_data = writer.take_data();

    // Compress data using zlib
    auto compressed_result = nbt_compression::compress_zlib(nbt_data);
    if (!compressed_result) {
        return compressed_result.error();
    }

    std::vector<u8> compressed_data = std::move(compressed_result.value());

    // Calculate required sectors
    // Data format: [length:4][compression:1][data:N]
    usize total_size = 4 + 1 + compressed_data.size();
    i32 required_sectors = (static_cast<i32>(total_size) + SECTOR_SIZE - 1) / SECTOR_SIZE;

    if (required_sectors >= 256) {
        return ErrorCode::InvalidArgument;  // Too large
    }

    // Get current offset
    i32 current_offset = get_offset(chunk_x, chunk_z);
    i32 current_sector_offset = current_offset >> 8;
    i32 current_sector_count = current_offset & 0xFF;

    i32 new_sector_offset;

    // Check if we can reuse existing sectors
    if (current_offset != 0 && current_sector_count == required_sectors) {
        // Reuse existing sectors
        new_sector_offset = current_sector_offset;
    } else {
        // Free old sectors
        if (current_offset != 0) {
            free_sectors(current_sector_offset, current_sector_count);
        }

        // Allocate new sectors
        auto allocate_result = allocate_sectors(required_sectors);
        if (!allocate_result) {
            return allocate_result.error();
        }
        new_sector_offset = allocate_result.value();
    }

    // Prepare data to write
    std::vector<u8> write_data;
    write_data.reserve(total_size);

    // Write length (big-endian)
    i32 data_length = static_cast<i32>(compressed_data.size()) + 1;
    write_data.push_back((data_length >> 24) & 0xFF);
    write_data.push_back((data_length >> 16) & 0xFF);
    write_data.push_back((data_length >> 8) & 0xFF);
    write_data.push_back(data_length & 0xFF);

    // Write compression type
    write_data.push_back(static_cast<u8>(CompressionType::ZLib));

    // Write compressed data
    write_data.insert(write_data.end(), compressed_data.begin(), compressed_data.end());

    // Write to file
    auto write_result = write_sectors(new_sector_offset, write_data);
    if (!write_result) {
        return write_result.error();
    }

    // Update offset
    i32 new_offset = (new_sector_offset << 8) | required_sectors;
    auto offset_result = set_offset(chunk_x, chunk_z, new_offset);
    if (!offset_result) {
        return offset_result;
    }

    // Update timestamp
    i32 timestamp = static_cast<i32>(std::time(nullptr));
    return set_timestamp(chunk_x, chunk_z, timestamp);
}

Result<i32> RegionFile::allocate_sectors(i32 sector_count) {
    // Find contiguous free sectors
    i32 start_sector = -1;
    i32 free_count = 0;

    for (usize i = 0; i < sectors_free_.size(); ++i) {
        if (sectors_free_[i]) {
            if (free_count == 0) {
                start_sector = static_cast<i32>(i);
            }
            free_count++;
            if (free_count >= sector_count) {
                // Found enough sectors
                for (i32 j = 0; j < sector_count; ++j) {
                    sectors_free_[start_sector + j] = false;
                }
                return start_sector;
            }
        } else {
            free_count = 0;
        }
    }

    // Need to grow the file
    start_sector = static_cast<i32>(sectors_free_.size());
    for (i32 i = 0; i < sector_count; ++i) {
        sectors_free_.push_back(false);
    }

    return start_sector;
}

void RegionFile::free_sectors(i32 sector_offset, i32 sector_count) {
    for (i32 i = 0; i < sector_count; ++i) {
        if (sector_offset + i < static_cast<i32>(sectors_free_.size())) {
            sectors_free_[sector_offset + i] = true;
        }
    }
}

Result<std::vector<u8>> RegionFile::read_sectors(i32 sector_offset, i32 sector_count) {
    if (!is_open_) {
        return ErrorCode::IOError;
    }

    std::vector<u8> data(sector_count * SECTOR_SIZE);
    file_.seekg(sector_offset * SECTOR_SIZE);
    file_.read(reinterpret_cast<char*>(data.data()), sector_count * SECTOR_SIZE);

    if (!file_) {
        return ErrorCode::IOError;
    }

    return data;
}

Result<void> RegionFile::write_sectors(i32 sector_offset, const std::vector<u8>& data) {
    if (!is_open_) {
        return ErrorCode::IOError;
    }

    // Seek to sector
    file_.seekp(sector_offset * SECTOR_SIZE);

    // Write data
    file_.write(reinterpret_cast<const char*>(data.data()), data.size());

    // Pad with zeros to sector boundary
    usize remainder = data.size() % SECTOR_SIZE;
    if (remainder != 0) {
        usize padding = SECTOR_SIZE - remainder;
        std::vector<u8> zeros(padding, 0);
        file_.write(reinterpret_cast<const char*>(zeros.data()), padding);
    }

    file_.flush();

    if (!file_) {
        return ErrorCode::IOError;
    }

    return Result<void>();
}

} // namespace mcserver
