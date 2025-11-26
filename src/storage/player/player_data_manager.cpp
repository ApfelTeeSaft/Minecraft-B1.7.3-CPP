#include "player_data_manager.hpp"
#include "storage/nbt/nbt_io.hpp"
#include "storage/async/async_io.hpp"
#include "util/log/logger.hpp"
#include <fstream>
#include <cstring>

namespace mcserver {

PlayerDataManager::PlayerDataManager(const std::string& world_path, AsyncIO* async_io)
    : player_data_dir_(std::filesystem::path(world_path) / "players")
    , async_io_(async_io) {
    auto result = ensure_directory_exists();
    if (!result) {
        LOG_ERROR_CAT("Failed to create player data directory",
                     LogCategory::Storage);
    }
}

Result<void> PlayerDataManager::save_player(const Player& player) {
    // Create player data directory if it doesn't exist
    auto dir_result = ensure_directory_exists();
    if (!dir_result) {
        return dir_result.error();
    }

    // Serialize player to NBT
    auto nbt = serialize_player(player);

    // Write NBT to buffer
    NBTWriter writer;
    writer.write_compound("", *nbt);
    auto data = writer.take_data();

    // Compress with gzip
    auto compressed = nbt_compression::compress_gzip(data);
    if (!compressed) {
        LOG_ERROR_CAT("Failed to compress player data", LogCategory::Storage);
        return compressed.error();
    }

    // Use UUID-based filename
    auto file_path = get_player_file_path(player.get_uuid());
    auto backup_path = get_player_backup_path(player.get_uuid());

    // Create backup if existing file exists
    if (std::filesystem::exists(file_path)) {
        std::error_code ec;
        std::filesystem::copy_file(file_path, backup_path,
                                  std::filesystem::copy_options::overwrite_existing, ec);
        if (ec) {
            LOG_WARNING_CAT("Failed to create backup before saving player data: " + ec.message(),
                        LogCategory::Storage);
            // Continue anyway - backup failure shouldn't prevent saving
        }
    }

    // Write to file
    std::ofstream file(file_path, std::ios::binary);
    if (!file.is_open()) {
        LOG_ERROR_CAT("Failed to open player data file for writing: " + file_path.string(),
                     LogCategory::Storage);
        return ErrorCode::IOError;
    }

    file.write(reinterpret_cast<const char*>(compressed.value().data()),
               compressed.value().size());
    file.close();

    if (!file.good()) {
        LOG_ERROR_CAT("Failed to write player data file: " + file_path.string(),
                     LogCategory::Storage);
        return ErrorCode::IOError;
    }

    LOG_DEBUG_CAT("Saved player data for " + player.get_username() +
                 " (UUID: " + player.get_uuid().to_string() + ", " +
                 std::to_string(compressed.value().size()) + " bytes)",
                 LogCategory::Storage);

    return {};
}

Result<bool> PlayerDataManager::load_player(Player& player) {
    // Try UUID-based file first
    auto file_path = get_player_file_path(player.get_uuid());
    bool using_legacy = false;

    // Check if UUID-based file exists
    if (!std::filesystem::exists(file_path)) {
        // Try legacy username-based file
        auto legacy_path = get_player_file_path_legacy(player.get_username());
        if (std::filesystem::exists(legacy_path)) {
            LOG_INFO_CAT("Found legacy player data file for " + player.get_username() +
                        ", will migrate after loading", LogCategory::Storage);
            file_path = legacy_path;
            using_legacy = true;
        } else {
            LOG_DEBUG_CAT("No existing player data for " + player.get_username(),
                         LogCategory::Storage);
            return false;  // No data exists, not an error
        }
    }

    // Helper lambda to load from a specific path
    auto load_from_path = [this, &player](const std::filesystem::path& path) -> Result<bool> {
        // Read file
        std::ifstream file(path, std::ios::binary | std::ios::ate);
        if (!file.is_open()) {
            LOG_ERROR_CAT("Failed to open player data file for reading: " + path.string(),
                         LogCategory::Storage);
            return ErrorCode::IOError;
        }

        usize file_size = file.tellg();
        file.seekg(0, std::ios::beg);

        std::vector<u8> compressed_data(file_size);
        file.read(reinterpret_cast<char*>(compressed_data.data()), file_size);
        file.close();

        if (!file.good()) {
            LOG_ERROR_CAT("Failed to read player data file: " + path.string(),
                         LogCategory::Storage);
            return ErrorCode::IOError;
        }

        // Decompress
        auto decompressed = nbt_compression::decompress_gzip(
            compressed_data.data(), compressed_data.size());
        if (!decompressed) {
            LOG_ERROR_CAT("Failed to decompress player data", LogCategory::Storage);
            return decompressed.error();
        }

        // Parse NBT
        NBTReader reader(decompressed.value());
        auto nbt = reader.read_compound();
        if (!nbt) {
            LOG_ERROR_CAT("Failed to parse player data NBT", LogCategory::Storage);
            return nbt.error();
        }

        // Deserialize to player
        auto deserialize_result = deserialize_player(player, *nbt.value());
        if (!deserialize_result) {
            return deserialize_result.error();
        }

        return true;
    };

    // Try to load from main file
    auto load_result = load_from_path(file_path);

    // If loading failed and we're using UUID file, try backup
    if (!load_result && !using_legacy) {
        LOG_WARNING_CAT("Failed to load player data, attempting backup restore",
                    LogCategory::Storage);

        auto backup_path = get_player_backup_path(player.get_uuid());
        if (std::filesystem::exists(backup_path)) {
            load_result = load_from_path(backup_path);
            if (load_result) {
                LOG_INFO_CAT("Successfully restored player data from backup",
                            LogCategory::Storage);
                // Copy backup to main file
                std::error_code ec;
                std::filesystem::copy_file(backup_path, file_path,
                                          std::filesystem::copy_options::overwrite_existing, ec);
            }
        }
    }

    if (!load_result) {
        return load_result.error();
    }

    // Migrate legacy file if necessary
    if (using_legacy) {
        auto migrate_result = migrate_legacy_file(player);
        if (!migrate_result) {
            LOG_WARNING_CAT("Failed to migrate legacy player data file", LogCategory::Storage);
            // Don't fail the load operation - data was loaded successfully
        }
    }

    LOG_INFO_CAT("Loaded player data for " + player.get_username() +
                 " (UUID: " + player.get_uuid().to_string() +
                 ", pos: " + std::to_string(player.get_x()) + ", " +
                 std::to_string(player.get_y()) + ", " +
                 std::to_string(player.get_z()) + ")",
                 LogCategory::Storage);

    return true;
}

bool PlayerDataManager::has_player_data(const std::string& username) const {
    return std::filesystem::exists(get_player_file_path_legacy(username));
}

bool PlayerDataManager::has_player_data_by_uuid(const UUID& uuid) const {
    return std::filesystem::exists(get_player_file_path(uuid));
}

Result<void> PlayerDataManager::delete_player_data(const std::string& username) {
    auto file_path = get_player_file_path_legacy(username);

    if (!std::filesystem::exists(file_path)) {
        return {};  // Already deleted, not an error
    }

    std::error_code ec;
    std::filesystem::remove(file_path, ec);

    if (ec) {
        LOG_ERROR_CAT("Failed to delete player data file: " + ec.message(),
                     LogCategory::Storage);
        return ErrorCode::IOError;
    }

    LOG_INFO_CAT("Deleted player data for " + username, LogCategory::Storage);
    return {};
}

Result<void> PlayerDataManager::delete_player_data_by_uuid(const UUID& uuid) {
    auto file_path = get_player_file_path(uuid);
    auto backup_path = get_player_backup_path(uuid);

    bool any_deleted = false;

    // Delete main file
    if (std::filesystem::exists(file_path)) {
        std::error_code ec;
        std::filesystem::remove(file_path, ec);
        if (ec) {
            LOG_ERROR_CAT("Failed to delete player data file: " + ec.message(),
                         LogCategory::Storage);
            return ErrorCode::IOError;
        }
        any_deleted = true;
    }

    // Delete backup file
    if (std::filesystem::exists(backup_path)) {
        std::error_code ec;
        std::filesystem::remove(backup_path, ec);
        if (ec) {
            LOG_WARNING_CAT("Failed to delete backup file: " + ec.message(),
                        LogCategory::Storage);
            // Don't fail if only backup deletion fails
        }
    }

    if (any_deleted) {
        LOG_INFO_CAT("Deleted player data for UUID: " + uuid.to_string(),
                    LogCategory::Storage);
    }

    return {};
}

Result<bool> PlayerDataManager::restore_from_backup(const UUID& uuid) {
    auto file_path = get_player_file_path(uuid);
    auto backup_path = get_player_backup_path(uuid);

    if (!std::filesystem::exists(backup_path)) {
        LOG_DEBUG_CAT("No backup file found for UUID: " + uuid.to_string(),
                     LogCategory::Storage);
        return false;
    }

    std::error_code ec;
    std::filesystem::copy_file(backup_path, file_path,
                              std::filesystem::copy_options::overwrite_existing, ec);

    if (ec) {
        LOG_ERROR_CAT("Failed to restore from backup: " + ec.message(),
                     LogCategory::Storage);
        return ErrorCode::IOError;
    }

    LOG_INFO_CAT("Restored player data from backup for UUID: " + uuid.to_string(),
                LogCategory::Storage);
    return true;
}

std::filesystem::path PlayerDataManager::get_player_file_path(const UUID& uuid) const {
    return player_data_dir_ / (uuid.to_filename() + ".dat");
}

std::filesystem::path PlayerDataManager::get_player_backup_path(const UUID& uuid) const {
    return player_data_dir_ / (uuid.to_filename() + ".dat.bak");
}

std::filesystem::path PlayerDataManager::get_player_file_path_legacy(const std::string& username) const {
    return player_data_dir_ / (username + ".dat");
}

Result<void> PlayerDataManager::migrate_legacy_file(const Player& player) {
    auto legacy_path = get_player_file_path_legacy(player.get_username());
    auto new_path = get_player_file_path(player.get_uuid());

    if (!std::filesystem::exists(legacy_path)) {
        return {};  // Nothing to migrate
    }

    // Copy legacy file to UUID-based filename
    std::error_code ec;
    std::filesystem::copy_file(legacy_path, new_path,
                              std::filesystem::copy_options::overwrite_existing, ec);

    if (ec) {
        LOG_ERROR_CAT("Failed to copy legacy file during migration: " + ec.message(),
                     LogCategory::Storage);
        return ErrorCode::IOError;
    }

    // Delete legacy file after successful copy
    std::filesystem::remove(legacy_path, ec);
    if (ec) {
        LOG_WARNING_CAT("Failed to delete legacy file after migration: " + ec.message(),
                    LogCategory::Storage);
        // Don't fail - the important part (copying to new location) succeeded
    }

    LOG_INFO_CAT("Migrated player data from " + legacy_path.filename().string() +
                " to " + new_path.filename().string(),
                LogCategory::Storage);

    return {};
}

std::unique_ptr<NBTCompound> PlayerDataManager::serialize_player(const Player& player) const {
    auto nbt = std::make_unique<NBTCompound>();

    // Position
    nbt->set_double("x", player.get_x());
    nbt->set_double("y", player.get_y());
    nbt->set_double("z", player.get_z());

    // Rotation
    nbt->set_float("yaw", player.get_yaw());
    nbt->set_float("pitch", player.get_pitch());

    // Health and food
    nbt->set_short("health", player.get_health());
    nbt->set_short("food", player.get_food());

    // Inventory
    auto inventory_list = std::make_unique<NBTList>(NBTType::Compound);
    const Inventory* inventory = player.get_inventory();

    for (i32 slot = 0; slot < inventory->size(); ++slot) {
        const ItemStack* stack = inventory->get_slot(slot);
        if (stack && !stack->is_empty()) {
            auto slot_nbt = std::make_unique<NBTCompound>();
            slot_nbt->set_byte("Slot", static_cast<i8>(slot));
            slot_nbt->set_short("id", stack->get_item_id());
            slot_nbt->set_byte("Count", stack->get_count());
            slot_nbt->set_short("Damage", stack->get_damage());
            inventory_list->add(std::move(slot_nbt));
        }
    }
    nbt->set_tag("Inventory", std::move(inventory_list));

    // Current hotbar slot
    nbt->set_byte("SelectedItemSlot", static_cast<i8>(inventory->get_current_slot()));

    return nbt;
}

Result<void> PlayerDataManager::deserialize_player(Player& player, const NBTCompound& nbt) const {
    // Position
    auto x = nbt.get_double("x");
    auto y = nbt.get_double("y");
    auto z = nbt.get_double("z");

    if (!x || !y || !z) {
        LOG_ERROR_CAT("Missing position data in player NBT", LogCategory::Storage);
        return ErrorCode::ParseError;
    }

    player.set_position(x.value(), y.value(), z.value());

    // Rotation
    auto yaw = nbt.get_float("yaw");
    auto pitch = nbt.get_float("pitch");

    if (yaw && pitch) {
        player.set_rotation(yaw.value(), pitch.value());
    }

    // Health and food
    auto health = nbt.get_short("health");
    auto food = nbt.get_short("food");

    if (health) {
        player.set_health(health.value());
    }
    if (food) {
        player.set_food(food.value());
    }

    // Inventory
    NBTList* inventory_list = nbt.get_list("Inventory");
    if (inventory_list) {
        Inventory* inventory = player.get_inventory();

        for (usize i = 0; i < inventory_list->size(); ++i) {
            NBTCompound* slot_nbt = dynamic_cast<NBTCompound*>(inventory_list->value[i].get());
            if (!slot_nbt) {
                continue;
            }

            auto slot_result = slot_nbt->get_byte("Slot");
            auto id_result = slot_nbt->get_short("id");
            auto count_result = slot_nbt->get_byte("Count");
            auto damage_result = slot_nbt->get_short("Damage");

            if (slot_result && id_result && count_result) {
                i32 slot = static_cast<i32>(slot_result.value());
                if (inventory->is_valid_slot(slot)) {
                    i16 damage = damage_result ? damage_result.value() : 0;
                    auto item_stack = std::make_unique<ItemStack>(
                        id_result.value(),
                        count_result.value(),
                        damage
                    );
                    inventory->set_slot(slot, std::move(item_stack));
                }
            }
        }
    }

    // Current hotbar slot
    auto selected_slot = nbt.get_byte("SelectedItemSlot");
    if (selected_slot) {
        player.get_inventory()->set_current_slot(static_cast<i32>(selected_slot.value()));
    }

    return {};
}

Result<void> PlayerDataManager::ensure_directory_exists() {
    if (std::filesystem::exists(player_data_dir_)) {
        return {};
    }

    std::error_code ec;
    std::filesystem::create_directories(player_data_dir_, ec);

    if (ec) {
        LOG_ERROR_CAT("Failed to create player data directory: " + ec.message(),
                     LogCategory::Storage);
        return ErrorCode::IOError;
    }

    LOG_INFO_CAT("Created player data directory: " + player_data_dir_.string(),
                LogCategory::Storage);
    return {};
}

void PlayerDataManager::save_player_async(const Player& player, SaveCallback callback) {
    if (!async_io_) {
        LOG_WARNING_CAT("Async I/O not available, falling back to synchronous save",
                       LogCategory::Storage);
        auto result = save_player(player);
        if (callback) {
            callback(std::move(result));
        }
        return;
    }

    // Create a copy of the player data for async operation
    // We need to serialize on the calling thread to ensure thread-safety
    // Use shared_ptr for async operation (std::function requires copy-constructible callables)
    auto nbt = std::shared_ptr<NBTCompound>(serialize_player(player));
    auto uuid = player.get_uuid();
    auto username = player.get_username();

    // Submit the async save task
    async_io_->submit_async(
        [this, nbt, uuid, username]() -> Result<void> {
            // Ensure directory exists
            auto dir_result = ensure_directory_exists();
            if (!dir_result) {
                return dir_result.error();
            }

            // Write NBT to buffer
            NBTWriter writer;
            writer.write_compound("", *nbt);
            auto data = writer.take_data();

            // Compress with gzip
            auto compressed = nbt_compression::compress_gzip(data);
            if (!compressed) {
                LOG_ERROR_CAT("Failed to compress player data", LogCategory::Storage);
                return compressed.error();
            }

            // Get file paths
            auto file_path = get_player_file_path(uuid);
            auto backup_path = get_player_backup_path(uuid);

            // Create backup if existing file exists
            if (std::filesystem::exists(file_path)) {
                std::error_code ec;
                std::filesystem::copy_file(file_path, backup_path,
                                          std::filesystem::copy_options::overwrite_existing, ec);
                if (ec) {
                    LOG_WARNING_CAT("Failed to create backup before saving player data: " + ec.message(),
                                   LogCategory::Storage);
                }
            }

            // Write to file
            std::ofstream file(file_path, std::ios::binary);
            if (!file.is_open()) {
                LOG_ERROR_CAT("Failed to open player data file for writing: " + file_path.string(),
                             LogCategory::Storage);
                return ErrorCode::IOError;
            }

            file.write(reinterpret_cast<const char*>(compressed.value().data()),
                      compressed.value().size());
            file.close();

            if (!file.good()) {
                LOG_ERROR_CAT("Failed to write player data file: " + file_path.string(),
                             LogCategory::Storage);
                return ErrorCode::IOError;
            }

            LOG_DEBUG_CAT("Saved player data for " + username +
                         " (UUID: " + uuid.to_string() + ", " +
                         std::to_string(compressed.value().size()) + " bytes) [async]",
                         LogCategory::Storage);

            return {};
        },
        callback
    );
}

void PlayerDataManager::load_player_async(Player& player, LoadCallback callback) {
    if (!async_io_) {
        LOG_WARNING_CAT("Async I/O not available, falling back to synchronous load",
                       LogCategory::Storage);
        auto result = load_player(player);
        if (callback) {
            callback(std::move(result));
        }
        return;
    }

    // IMPORTANT: The Player object must remain valid until the callback is invoked
    // The caller is responsible for ensuring this

    auto uuid = player.get_uuid();
    auto username = player.get_username();

    async_io_->submit_async<bool>(
        [this, &player, uuid, username]() -> Result<bool> {
            // Try UUID-based file first
            auto file_path = get_player_file_path(uuid);
            bool using_legacy = false;

            // Check if UUID-based file exists
            if (!std::filesystem::exists(file_path)) {
                // Try legacy username-based file
                auto legacy_path = get_player_file_path_legacy(username);
                if (std::filesystem::exists(legacy_path)) {
                    LOG_INFO_CAT("Found legacy player data file for " + username +
                                ", will migrate after loading", LogCategory::Storage);
                    file_path = legacy_path;
                    using_legacy = true;
                } else {
                    LOG_DEBUG_CAT("No existing player data for " + username,
                                 LogCategory::Storage);
                    return false;
                }
            }

            // Helper lambda to load from a specific path
            auto load_from_path = [this, &player](const std::filesystem::path& path) -> Result<bool> {
                std::ifstream file(path, std::ios::binary | std::ios::ate);
                if (!file.is_open()) {
                    LOG_ERROR_CAT("Failed to open player data file for reading: " + path.string(),
                                 LogCategory::Storage);
                    return ErrorCode::IOError;
                }

                usize file_size = file.tellg();
                file.seekg(0, std::ios::beg);

                std::vector<u8> compressed_data(file_size);
                file.read(reinterpret_cast<char*>(compressed_data.data()), file_size);
                file.close();

                if (!file.good()) {
                    LOG_ERROR_CAT("Failed to read player data file: " + path.string(),
                                 LogCategory::Storage);
                    return ErrorCode::IOError;
                }

                auto decompressed = nbt_compression::decompress_gzip(
                    compressed_data.data(), compressed_data.size());
                if (!decompressed) {
                    LOG_ERROR_CAT("Failed to decompress player data", LogCategory::Storage);
                    return decompressed.error();
                }

                NBTReader reader(decompressed.value());
                auto nbt = reader.read_compound();
                if (!nbt) {
                    LOG_ERROR_CAT("Failed to parse player data NBT", LogCategory::Storage);
                    return nbt.error();
                }

                auto deserialize_result = deserialize_player(player, *nbt.value());
                if (!deserialize_result) {
                    return deserialize_result.error();
                }

                return true;
            };

            // Try to load from main file
            auto load_result = load_from_path(file_path);

            // If loading failed and we're using UUID file, try backup
            if (!load_result && !using_legacy) {
                LOG_WARNING_CAT("Failed to load player data, attempting backup restore",
                               LogCategory::Storage);

                auto backup_path = get_player_backup_path(uuid);
                if (std::filesystem::exists(backup_path)) {
                    load_result = load_from_path(backup_path);
                    if (load_result) {
                        LOG_INFO_CAT("Successfully restored player data from backup",
                                    LogCategory::Storage);
                        std::error_code ec;
                        std::filesystem::copy_file(backup_path, file_path,
                                                  std::filesystem::copy_options::overwrite_existing, ec);
                    }
                }
            }

            if (!load_result) {
                return load_result.error();
            }

            // Migrate legacy file if necessary
            if (using_legacy) {
                auto migrate_result = migrate_legacy_file(player);
                if (!migrate_result) {
                    LOG_WARNING_CAT("Failed to migrate legacy player data file", LogCategory::Storage);
                }
            }

            LOG_INFO_CAT("Loaded player data for " + username +
                        " (UUID: " + uuid.to_string() +
                        ", pos: " + std::to_string(player.get_x()) + ", " +
                        std::to_string(player.get_y()) + ", " +
                        std::to_string(player.get_z()) + ") [async]",
                        LogCategory::Storage);

            return true;
        },
        callback
    );
}

} // namespace mcserver
