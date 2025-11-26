#include "admin_manager.hpp"
#include "entity/player.hpp"
#include "entity/entity_manager.hpp"
#include "world/chunk/chunk_manager.hpp"
#include "util/log/logger.hpp"
#include <fstream>
#include <sstream>
#include <algorithm>

namespace mcserver {

AdminManager::AdminManager() {
    // Auto-add default admin
    add_admin("apfelteesaft_");

    // Register built-in commands
    register_builtin_commands();
}

void AdminManager::add_admin(const std::string& username) {
    admins_.insert(username);
    LOG_INFO_CAT("Added admin: " + username, LogCategory::General);
}

void AdminManager::remove_admin(const std::string& username) {
    // Protect default admin
    if (username == "apfelteesaft_") {
        LOG_WARNING_CAT("Cannot remove default admin: " + username, LogCategory::General);
        return;
    }

    admins_.erase(username);
    LOG_INFO_CAT("Removed admin: " + username, LogCategory::General);
}

bool AdminManager::is_admin(const std::string& username) const {
    return admins_.find(username) != admins_.end();
}

void AdminManager::save_admin_list(const std::string& file_path) {
    std::ofstream file(file_path);
    if (!file.is_open()) {
        LOG_ERROR_CAT("Failed to save admin list to: " + file_path, LogCategory::General);
        return;
    }

    for (const auto& admin : admins_) {
        file << admin << "\n";
    }

    file.close();
    LOG_INFO_CAT("Saved " + std::to_string(admins_.size()) + " admins to: " + file_path,
                LogCategory::General);
}

void AdminManager::load_admin_list(const std::string& file_path) {
    std::ifstream file(file_path);
    if (!file.is_open()) {
        LOG_INFO_CAT("No existing admin list found at: " + file_path, LogCategory::General);
        return;
    }

    std::string username;
    while (std::getline(file, username)) {
        if (!username.empty()) {
            admins_.insert(username);
        }
    }

    file.close();
    LOG_INFO_CAT("Loaded " + std::to_string(admins_.size()) + " admins from: " + file_path,
                LogCategory::General);
}

void AdminManager::register_command(const std::string& name, CommandHandler handler, const std::string& usage) {
    commands_[name] = handler;
    if (!usage.empty()) {
        command_usage_[name] = usage;
    }
}

CommandResult AdminManager::execute_command(const std::string& command, Player* player) {
    if (!player) {
        return CommandResult::error("Invalid player");
    }

    // Check admin permission
    if (!is_admin(player->get_username())) {
        return CommandResult::error("§cYou don't have permission to use this command");
    }

    // Parse command
    std::istringstream iss(command);
    std::vector<std::string> tokens;
    std::string token;

    while (iss >> token) {
        tokens.push_back(token);
    }

    if (tokens.empty()) {
        return CommandResult::error("Empty command");
    }

    // Remove '/' prefix if present
    std::string cmd_name = tokens[0];
    if (cmd_name[0] == '/') {
        cmd_name = cmd_name.substr(1);
    }

    // Get command arguments
    std::vector<std::string> args(tokens.begin() + 1, tokens.end());

    // Find and execute command
    auto it = commands_.find(cmd_name);
    if (it == commands_.end()) {
        return CommandResult::error("§cUnknown command: /" + cmd_name);
    }

    return it->second(player, args);
}

void AdminManager::register_builtin_commands() {
    register_command("fly", [this](Player* p, const std::vector<std::string>& args) {
        return cmd_fly(p, args);
    }, "/fly - Toggle flight mode");

    register_command("give", [this](Player* p, const std::vector<std::string>& args) {
        return cmd_give(p, args);
    }, "/give <item_id> [amount] - Give yourself items");

    register_command("tp", [this](Player* p, const std::vector<std::string>& args) {
        return cmd_tp(p, args);
    }, "/tp <x> <y> <z> - Teleport to coordinates");

    register_command("gamemode", [this](Player* p, const std::vector<std::string>& args) {
        return cmd_gamemode(p, args);
    }, "/gamemode <0|1> - Change game mode (0=survival, 1=creative)");

    register_command("time", [this](Player* p, const std::vector<std::string>& args) {
        return cmd_time(p, args);
    }, "/time <set|add> <value> - Change world time");

    register_command("admin", [this](Player* p, const std::vector<std::string>& args) {
        return cmd_admin(p, args);
    }, "/admin <add|remove|list> [player] - Manage admins");

    register_command("help", [this](Player* p, const std::vector<std::string>& args) {
        return cmd_help(p, args);
    }, "/help - Show available commands");
}

CommandResult AdminManager::cmd_fly(Player* player, const std::vector<std::string>& args) {
    (void)player;  // Unused for now
    (void)args;    // Unused for now
    // Toggle flying ability
    // Note: Beta 1.7.3 doesn't have native flying, but we can allow creative-style flying
    // This would need to be implemented in the player movement handling
    return CommandResult::ok("§aFlight mode toggled (not yet fully implemented)");
}

CommandResult AdminManager::cmd_give(Player* player, const std::vector<std::string>& args) {
    if (args.empty()) {
        return CommandResult::error("§cUsage: /give <item_id> [amount]");
    }

    // Simple validation - check if string is numeric
    if (args[0].empty() || (!std::isdigit(args[0][0]) && args[0][0] != '-')) {
        return CommandResult::error("§cInvalid item ID: " + args[0]);
    }

    i16 item_id = static_cast<i16>(std::atoi(args[0].c_str()));

    i8 amount = 64;
    if (args.size() >= 2) {
        if (args[1].empty() || (!std::isdigit(args[1][0]) && args[1][0] != '-')) {
            return CommandResult::error("§cInvalid amount: " + args[1]);
        }
        amount = static_cast<i8>(std::atoi(args[1].c_str()));
        if (amount <= 0 || amount > 64) {
            return CommandResult::error("§cAmount must be between 1 and 64");
        }
    }

    // Add item to player inventory
    Inventory* inv = player->get_inventory();
    if (!inv) {
        return CommandResult::error("§cInventory not available");
    }

    // Use the inventory's add_item method which correctly places items in slots 0-35
    // (hotbar + main inventory), avoiding armor slots (36-39) and crafting slots (40-44)
    auto item = std::make_unique<ItemStack>(item_id, amount, static_cast<i16>(0));
    i8 remaining = inv->add_item(std::move(item));

    if (remaining == 0) {
        return CommandResult::ok("§aGave " + std::to_string(amount) + "x item " + std::to_string(item_id));
    } else if (remaining < amount) {
        i8 added = amount - remaining;
        return CommandResult::ok("§aGave " + std::to_string(added) + "x item " + std::to_string(item_id) +
                                " (§c" + std::to_string(remaining) + " couldn't fit§a)");
    } else {
        return CommandResult::error("§cInventory is full");
    }
}

CommandResult AdminManager::cmd_tp(Player* player, const std::vector<std::string>& args) {
    if (args.size() < 3) {
        return CommandResult::error("§cUsage: /tp <x> <y> <z>");
    }

    // Simple validation - just use atof which returns 0.0 on error (good enough for coordinates)
    f64 x = std::atof(args[0].c_str());
    f64 y = std::atof(args[1].c_str());
    f64 z = std::atof(args[2].c_str());

    player->set_position(x, y, z);
    return CommandResult::ok("§aTeleported to " + std::to_string(x) + ", " +
                            std::to_string(y) + ", " + std::to_string(z));
}

CommandResult AdminManager::cmd_gamemode(Player* player, const std::vector<std::string>& args) {
    (void)player;  // Unused in Beta 1.7.3 (no gamemode state to set)

    if (args.empty()) {
        return CommandResult::error("§cUsage: /gamemode <0|1>");
    }

    // Beta 1.7.3 doesn't have official game modes, but we can track this for future features
    if (args[0] == "0") {
        return CommandResult::ok("§aSet game mode to Survival (not fully implemented in Beta 1.7.3)");
    } else if (args[0] == "1") {
        return CommandResult::ok("§aSet game mode to Creative (not fully implemented in Beta 1.7.3)");
    } else {
        return CommandResult::error("§cInvalid game mode. Use 0 for Survival or 1 for Creative");
    }
}

CommandResult AdminManager::cmd_time(Player* player, const std::vector<std::string>& args) {
    (void)player;  // Unused - time is a world-level property

    if (args.size() < 2) {
        return CommandResult::error("§cUsage: /time <set|add> <value>");
    }

    // This would need to be implemented in the world/server time system
    // For now, just return a placeholder
    return CommandResult::ok("§aTime command received (world time system not yet implemented)");
}

CommandResult AdminManager::cmd_admin(Player* player, const std::vector<std::string>& args) {
    (void)player;  // Unused - admin management is global

    if (args.empty()) {
        return CommandResult::error("§cUsage: /admin <add|remove|list> [player]");
    }

    const std::string& subcmd = args[0];

    if (subcmd == "list") {
        std::string admin_list = "§aAdmins: ";
        for (const auto& admin : admins_) {
            admin_list += admin + ", ";
        }
        if (admin_list.size() > 10) {
            admin_list = admin_list.substr(0, admin_list.size() - 2);  // Remove trailing ", "
        }
        return CommandResult::ok(admin_list);
    }

    if (args.size() < 2) {
        return CommandResult::error("§cUsage: /admin " + subcmd + " <player>");
    }

    const std::string& target = args[1];

    if (subcmd == "add") {
        add_admin(target);
        save_admin_list("admins.txt");
        return CommandResult::ok("§aAdded " + target + " to admins");
    } else if (subcmd == "remove") {
        if (target == "apfelteesaft_") {
            return CommandResult::error("§cCannot remove default admin");
        }
        remove_admin(target);
        save_admin_list("admins.txt");
        return CommandResult::ok("§aRemoved " + target + " from admins");
    } else {
        return CommandResult::error("§cUnknown subcommand: " + subcmd);
    }
}

CommandResult AdminManager::cmd_help(Player* player, const std::vector<std::string>& args) {
    (void)player;  // Unused
    (void)args;    // Unused

    std::string help_text = "§aAvailable admin commands:\n";
    for (const auto& [name, usage] : command_usage_) {
        help_text += "§e" + usage + "\n";
    }
    return CommandResult::ok(help_text);
}

} // namespace mcserver
