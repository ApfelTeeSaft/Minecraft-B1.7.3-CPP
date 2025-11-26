#pragma once

#include "entity/player.hpp"
#include "storage/nbt/nbt.hpp"
#include "util/result.hpp"
#include "util/types.hpp"
#include <string>
#include <filesystem>
#include <functional>

namespace mcserver {

class AsyncIO;

// Manages saving and loading player data to/from disk
// Player data is stored in NBT format in world/players/{UUID}.dat
// Supports both synchronous and asynchronous operations
class PlayerDataManager {
public:
    using SaveCallback = std::function<void(Result<void>)>;
    using LoadCallback = std::function<void(Result<bool>)>;

    explicit PlayerDataManager(const std::string& world_path, AsyncIO* async_io = nullptr);

    // Synchronous operations
    // Save player data to disk
    Result<void> save_player(const Player& player);

    // Load player data from disk
    // Returns true if data was found and loaded, false if no data exists
    Result<bool> load_player(Player& player);

    // Asynchronous operations (requires AsyncIO)
    // Save player data asynchronously
    void save_player_async(const Player& player, SaveCallback callback = nullptr);

    // Load player data asynchronously
    // Note: Player object must remain valid until callback is invoked
    void load_player_async(Player& player, LoadCallback callback = nullptr);

    // Check if player data exists on disk
    bool has_player_data(const std::string& username) const;
    bool has_player_data_by_uuid(const UUID& uuid) const;

    // Delete player data file (for admin commands)
    Result<void> delete_player_data(const std::string& username);
    Result<void> delete_player_data_by_uuid(const UUID& uuid);

    // Restore from backup if available
    Result<bool> restore_from_backup(const UUID& uuid);

private:
    std::filesystem::path player_data_dir_;
    AsyncIO* async_io_;  // Optional async I/O for non-blocking operations

    // Get path to player data file (UUID-based)
    std::filesystem::path get_player_file_path(const UUID& uuid) const;
    std::filesystem::path get_player_backup_path(const UUID& uuid) const;

    // Legacy: Get path from username (for migration)
    std::filesystem::path get_player_file_path_legacy(const std::string& username) const;

    // Migrate legacy username-based file to UUID-based
    Result<void> migrate_legacy_file(const Player& player);

    // Serialize player to NBT
    std::unique_ptr<NBTCompound> serialize_player(const Player& player) const;

    // Deserialize NBT to player
    Result<void> deserialize_player(Player& player, const NBTCompound& nbt) const;

    // Ensure player data directory exists
    Result<void> ensure_directory_exists();
};

} // namespace mcserver
