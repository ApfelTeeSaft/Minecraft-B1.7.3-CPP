#pragma once

#include "util/types.hpp"
#include <string>
#include <unordered_set>
#include <unordered_map>
#include <mutex>

namespace mcserver {

class Plugin;

// Plugin permission categories
enum class PluginPermission {
    // World permissions
    READ_BLOCKS,
    WRITE_BLOCKS,
    READ_CHUNKS,
    GENERATE_CHUNKS,
    MODIFY_LIGHTING,

    // Entity permissions
    READ_ENTITIES,
    SPAWN_ENTITIES,
    MODIFY_ENTITIES,
    DAMAGE_ENTITIES,
    KILL_ENTITIES,

    // Player permissions
    READ_PLAYER_DATA,
    MODIFY_INVENTORY,
    KICK_PLAYERS,
    BAN_PLAYERS,
    TELEPORT_PLAYERS,
    SEND_MESSAGES,

    // Network permissions
    SEND_PACKETS,
    INTERCEPT_PACKETS,

    // System permissions
    FILE_READ,
    FILE_WRITE,
    NETWORK_ACCESS,
    EXECUTE_COMMANDS,

    // Admin permissions
    RELOAD_PLUGINS,
    STOP_SERVER,
    MODIFY_CONFIG
};

// Convert permission to string for logging
inline std::string permission_to_string(PluginPermission perm) {
    switch (perm) {
        case PluginPermission::READ_BLOCKS: return "READ_BLOCKS";
        case PluginPermission::WRITE_BLOCKS: return "WRITE_BLOCKS";
        case PluginPermission::READ_CHUNKS: return "READ_CHUNKS";
        case PluginPermission::GENERATE_CHUNKS: return "GENERATE_CHUNKS";
        case PluginPermission::MODIFY_LIGHTING: return "MODIFY_LIGHTING";
        case PluginPermission::READ_ENTITIES: return "READ_ENTITIES";
        case PluginPermission::SPAWN_ENTITIES: return "SPAWN_ENTITIES";
        case PluginPermission::MODIFY_ENTITIES: return "MODIFY_ENTITIES";
        case PluginPermission::DAMAGE_ENTITIES: return "DAMAGE_ENTITIES";
        case PluginPermission::KILL_ENTITIES: return "KILL_ENTITIES";
        case PluginPermission::READ_PLAYER_DATA: return "READ_PLAYER_DATA";
        case PluginPermission::MODIFY_INVENTORY: return "MODIFY_INVENTORY";
        case PluginPermission::KICK_PLAYERS: return "KICK_PLAYERS";
        case PluginPermission::BAN_PLAYERS: return "BAN_PLAYERS";
        case PluginPermission::TELEPORT_PLAYERS: return "TELEPORT_PLAYERS";
        case PluginPermission::SEND_MESSAGES: return "SEND_MESSAGES";
        case PluginPermission::SEND_PACKETS: return "SEND_PACKETS";
        case PluginPermission::INTERCEPT_PACKETS: return "INTERCEPT_PACKETS";
        case PluginPermission::FILE_READ: return "FILE_READ";
        case PluginPermission::FILE_WRITE: return "FILE_WRITE";
        case PluginPermission::NETWORK_ACCESS: return "NETWORK_ACCESS";
        case PluginPermission::EXECUTE_COMMANDS: return "EXECUTE_COMMANDS";
        case PluginPermission::RELOAD_PLUGINS: return "RELOAD_PLUGINS";
        case PluginPermission::STOP_SERVER: return "STOP_SERVER";
        case PluginPermission::MODIFY_CONFIG: return "MODIFY_CONFIG";
        default: return "UNKNOWN";
    }
}

// Parse permission from string
inline bool string_to_permission(const std::string& str, PluginPermission& out) {
    static std::unordered_map<std::string, PluginPermission> perm_map = {
        {"READ_BLOCKS", PluginPermission::READ_BLOCKS},
        {"WRITE_BLOCKS", PluginPermission::WRITE_BLOCKS},
        {"READ_CHUNKS", PluginPermission::READ_CHUNKS},
        {"GENERATE_CHUNKS", PluginPermission::GENERATE_CHUNKS},
        {"MODIFY_LIGHTING", PluginPermission::MODIFY_LIGHTING},
        {"READ_ENTITIES", PluginPermission::READ_ENTITIES},
        {"SPAWN_ENTITIES", PluginPermission::SPAWN_ENTITIES},
        {"MODIFY_ENTITIES", PluginPermission::MODIFY_ENTITIES},
        {"DAMAGE_ENTITIES", PluginPermission::DAMAGE_ENTITIES},
        {"KILL_ENTITIES", PluginPermission::KILL_ENTITIES},
        {"READ_PLAYER_DATA", PluginPermission::READ_PLAYER_DATA},
        {"MODIFY_INVENTORY", PluginPermission::MODIFY_INVENTORY},
        {"KICK_PLAYERS", PluginPermission::KICK_PLAYERS},
        {"BAN_PLAYERS", PluginPermission::BAN_PLAYERS},
        {"TELEPORT_PLAYERS", PluginPermission::TELEPORT_PLAYERS},
        {"SEND_MESSAGES", PluginPermission::SEND_MESSAGES},
        {"SEND_PACKETS", PluginPermission::SEND_PACKETS},
        {"INTERCEPT_PACKETS", PluginPermission::INTERCEPT_PACKETS},
        {"FILE_READ", PluginPermission::FILE_READ},
        {"FILE_WRITE", PluginPermission::FILE_WRITE},
        {"NETWORK_ACCESS", PluginPermission::NETWORK_ACCESS},
        {"EXECUTE_COMMANDS", PluginPermission::EXECUTE_COMMANDS},
        {"RELOAD_PLUGINS", PluginPermission::RELOAD_PLUGINS},
        {"STOP_SERVER", PluginPermission::STOP_SERVER},
        {"MODIFY_CONFIG", PluginPermission::MODIFY_CONFIG}
    };

    auto it = perm_map.find(str);
    if (it != perm_map.end()) {
        out = it->second;
        return true;
    }
    return false;
}

// Manages plugin permissions
class PluginPermissionManager {
public:
    PluginPermissionManager() = default;

    // Grant a permission to a plugin
    void grant_permission(const std::string& plugin_name, PluginPermission perm) {
        std::lock_guard<std::mutex> lock(mutex_);
        permissions_[plugin_name].insert(perm);
    }

    // Revoke a permission from a plugin
    void revoke_permission(const std::string& plugin_name, PluginPermission perm) {
        std::lock_guard<std::mutex> lock(mutex_);
        auto it = permissions_.find(plugin_name);
        if (it != permissions_.end()) {
            it->second.erase(perm);
        }
    }

    // Check if plugin has a specific permission
    bool has_permission(const std::string& plugin_name, PluginPermission perm) const {
        std::lock_guard<std::mutex> lock(mutex_);
        auto it = permissions_.find(plugin_name);
        if (it != permissions_.end()) {
            return it->second.find(perm) != it->second.end();
        }
        return false;
    }

    // Grant all permissions (for trusted plugins)
    void grant_all_permissions(const std::string& plugin_name) {
        std::lock_guard<std::mutex> lock(mutex_);
        auto& perms = permissions_[plugin_name];
        perms.insert(PluginPermission::READ_BLOCKS);
        perms.insert(PluginPermission::WRITE_BLOCKS);
        perms.insert(PluginPermission::READ_CHUNKS);
        perms.insert(PluginPermission::GENERATE_CHUNKS);
        perms.insert(PluginPermission::MODIFY_LIGHTING);
        perms.insert(PluginPermission::READ_ENTITIES);
        perms.insert(PluginPermission::SPAWN_ENTITIES);
        perms.insert(PluginPermission::MODIFY_ENTITIES);
        perms.insert(PluginPermission::DAMAGE_ENTITIES);
        perms.insert(PluginPermission::KILL_ENTITIES);
        perms.insert(PluginPermission::READ_PLAYER_DATA);
        perms.insert(PluginPermission::MODIFY_INVENTORY);
        perms.insert(PluginPermission::KICK_PLAYERS);
        perms.insert(PluginPermission::BAN_PLAYERS);
        perms.insert(PluginPermission::TELEPORT_PLAYERS);
        perms.insert(PluginPermission::SEND_MESSAGES);
        perms.insert(PluginPermission::SEND_PACKETS);
        perms.insert(PluginPermission::INTERCEPT_PACKETS);
        perms.insert(PluginPermission::FILE_READ);
        perms.insert(PluginPermission::FILE_WRITE);
        perms.insert(PluginPermission::NETWORK_ACCESS);
        perms.insert(PluginPermission::EXECUTE_COMMANDS);
        perms.insert(PluginPermission::RELOAD_PLUGINS);
        perms.insert(PluginPermission::STOP_SERVER);
        perms.insert(PluginPermission::MODIFY_CONFIG);
    }

    // Revoke all permissions
    void revoke_all_permissions(const std::string& plugin_name) {
        std::lock_guard<std::mutex> lock(mutex_);
        permissions_.erase(plugin_name);
    }

    // Get all permissions for a plugin
    std::unordered_set<PluginPermission> get_permissions(const std::string& plugin_name) const {
        std::lock_guard<std::mutex> lock(mutex_);
        auto it = permissions_.find(plugin_name);
        if (it != permissions_.end()) {
            return it->second;
        }
        return {};
    }

private:
    mutable std::mutex mutex_;
    std::unordered_map<std::string, std::unordered_set<PluginPermission>> permissions_;
};

} // namespace mcserver

// Hash function for PluginPermission to use in unordered containers
namespace std {
    template<>
    struct hash<mcserver::PluginPermission> {
        size_t operator()(mcserver::PluginPermission perm) const {
            return hash<int>()(static_cast<int>(perm));
        }
    };
}
