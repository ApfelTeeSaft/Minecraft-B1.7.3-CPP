#include "chunk_manager.hpp"
#include "storage/chunk/chunk_storage.hpp"
#include "util/log/logger.hpp"

namespace mcserver {

ChunkManager::ChunkManager(WorldGenerator* generator, ChunkStorage* storage)
    : generator_(generator), storage_(storage) {}

Chunk* ChunkManager::get_chunk(i32 chunk_x, i32 chunk_z) {
    return load_chunk(chunk_x, chunk_z);
}

Chunk* ChunkManager::get_chunk_if_loaded(i32 chunk_x, i32 chunk_z) {
    auto key = make_key(chunk_x, chunk_z);
    auto it = chunks_.find(key);
    if (it != chunks_.end()) {
        return it->second.get();
    }
    return nullptr;
}

Chunk* ChunkManager::load_chunk(i32 chunk_x, i32 chunk_z) {
    auto key = make_key(chunk_x, chunk_z);

    // Check if already loaded
    auto it = chunks_.find(key);
    if (it != chunks_.end()) {
        return it->second.get();
    }

    std::unique_ptr<Chunk> chunk;

    // Try to load from storage first
    if (storage_ && storage_->chunk_exists(chunk_x, chunk_z)) {
        auto load_result = storage_->load_chunk(chunk_x, chunk_z);
        if (load_result) {
            chunk = std::move(load_result.value());
            Logger::instance().log(LogLevel::Debug, LogCategory::World,
                "Loaded chunk from disk (" + std::to_string(chunk_x) + ", " + std::to_string(chunk_z) + ")");
        } else {
            Logger::instance().log(LogLevel::Warning, LogCategory::World,
                "Failed to load chunk from disk, generating new (" + std::to_string(chunk_x) + ", " + std::to_string(chunk_z) + ")");
        }
    }

    // If not loaded from storage, create new chunk
    if (!chunk) {
        chunk = std::make_unique<Chunk>(chunk_x, chunk_z);

        // Generate terrain if generator is available
        if (generator_ && !chunk->is_generated()) {
            generator_->generate_chunk(*chunk);
            // Logger::instance().log(LogLevel::Debug, LogCategory::World,
            //     "Generated new chunk (" + std::to_string(chunk_x) + ", " + std::to_string(chunk_z) + ")");
        }
    }

    Chunk* chunk_ptr = chunk.get();
    chunks_[key] = std::move(chunk);

    return chunk_ptr;
}

void ChunkManager::unload_chunk(i32 chunk_x, i32 chunk_z) {
    auto key = make_key(chunk_x, chunk_z);
    auto it = chunks_.find(key);

    if (it != chunks_.end()) {
        // Save chunk if dirty and storage is available
        if (storage_ && it->second->is_dirty()) {
            auto save_result = storage_->save_chunk(*it->second);
            if (save_result) {
                Logger::instance().log(LogLevel::Debug, LogCategory::World,
                    "Saved chunk (" + std::to_string(chunk_x) + ", " + std::to_string(chunk_z) + ")");
            } else {
                Logger::instance().log(LogLevel::Error, LogCategory::World,
                    "Failed to save chunk (" + std::to_string(chunk_x) + ", " + std::to_string(chunk_z) + ")");
            }
        }

        Logger::instance().log(LogLevel::Debug, LogCategory::World,
            "Unloaded chunk (" + std::to_string(chunk_x) + ", " + std::to_string(chunk_z) + ")");
        chunks_.erase(it);
    }
}

bool ChunkManager::is_chunk_loaded(i32 chunk_x, i32 chunk_z) const {
    auto key = make_key(chunk_x, chunk_z);
    return chunks_.find(key) != chunks_.end();
}

std::vector<Chunk*> ChunkManager::get_loaded_chunks() {
    std::vector<Chunk*> result;
    result.reserve(chunks_.size());

    for (auto& [key, chunk] : chunks_) {
        result.push_back(chunk.get());
    }

    return result;
}

void ChunkManager::save_all_dirty() {
    if (!storage_) {
        return;
    }

    for (auto& [key, chunk] : chunks_) {
        if (chunk->is_dirty()) {
            auto save_result = storage_->save_chunk(*chunk);
            if (save_result) {
                chunk->clear_dirty();
                Logger::instance().log(LogLevel::Debug, LogCategory::World,
                    "Saved dirty chunk (" + std::to_string(chunk->get_x()) + ", " + std::to_string(chunk->get_z()) + ")");
            } else {
                Logger::instance().log(LogLevel::Error, LogCategory::World,
                    "Failed to save chunk (" + std::to_string(chunk->get_x()) + ", " + std::to_string(chunk->get_z()) + ")");
            }
        }
    }
}

void ChunkManager::save_all() {
    if (!storage_) {
        return;
    }

    for (auto& [key, chunk] : chunks_) {
        auto save_result = storage_->save_chunk(*chunk);
        if (save_result) {
            chunk->clear_dirty();
            Logger::instance().log(LogLevel::Debug, LogCategory::World,
                "Saved chunk (" + std::to_string(chunk->get_x()) + ", " + std::to_string(chunk->get_z()) + ")");
        } else {
            Logger::instance().log(LogLevel::Error, LogCategory::World,
                "Failed to save chunk (" + std::to_string(chunk->get_x()) + ", " + std::to_string(chunk->get_z()) + ")");
        }
    }
}

void ChunkManager::tick() {
    // Future: Handle chunk updates, lighting, etc.
}

} // namespace mcserver
