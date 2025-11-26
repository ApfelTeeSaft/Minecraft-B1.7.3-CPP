#pragma once

#include "util/types.hpp"
#include <string>
#include <unordered_set>
#include <functional>

namespace mcserver {

class Player;
class ChunkManager;
class EntityManager;
class MobManager;

// Admin command result
struct CommandResult {
    bool success;
    std::string message;

    static CommandResult ok(const std::string& msg = "") {
        return {true, msg};
    }

    static CommandResult error(const std::string& msg) {
        return {false, msg};
    }
};

// Admin command handler
using CommandHandler = std::function<CommandResult(Player*, const std::vector<std::string>&)>;

// Manages admin permissions and commands
class AdminManager {
public:
    AdminManager();

    // Admin management
    void add_admin(const std::string& username);
    void remove_admin(const std::string& username);
    bool is_admin(const std::string& username) const;
    void save_admin_list(const std::string& file_path);
    void load_admin_list(const std::string& file_path);

    // Command registration
    void register_command(const std::string& name, CommandHandler handler, const std::string& usage = "");

    // Command execution
    CommandResult execute_command(const std::string& command, Player* player);

    // Set manager references for commands
    void set_chunk_manager(ChunkManager* manager) { chunk_manager_ = manager; }
    void set_entity_manager(EntityManager* manager) { entity_manager_ = manager; }
    void set_mob_manager(MobManager* manager) { mob_manager_ = manager; }

    ChunkManager* get_chunk_manager() { return chunk_manager_; }
    EntityManager* get_entity_manager() { return entity_manager_; }
    MobManager* get_mob_manager() { return mob_manager_; }

private:
    std::unordered_set<std::string> admins_;
    std::unordered_map<std::string, CommandHandler> commands_;
    std::unordered_map<std::string, std::string> command_usage_;

    ChunkManager* chunk_manager_ = nullptr;
    EntityManager* entity_manager_ = nullptr;
    MobManager* mob_manager_ = nullptr;

    // Built-in commands
    void register_builtin_commands();
    CommandResult cmd_fly(Player* player, const std::vector<std::string>& args);
    CommandResult cmd_give(Player* player, const std::vector<std::string>& args);
    CommandResult cmd_tp(Player* player, const std::vector<std::string>& args);
    CommandResult cmd_gamemode(Player* player, const std::vector<std::string>& args);
    CommandResult cmd_time(Player* player, const std::vector<std::string>& args);
    CommandResult cmd_admin(Player* player, const std::vector<std::string>& args);
    CommandResult cmd_help(Player* player, const std::vector<std::string>& args);
};

} // namespace mcserver
