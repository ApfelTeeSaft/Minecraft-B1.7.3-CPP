#pragma once

#include <string>
#include <map>
#include "util/types.hpp"
#include "util/result.hpp"

namespace mcserver {

class ServerConfig {
public:
    ServerConfig() = default;

    // Load from server.properties file
    // If file doesn't exist, will populate with defaults and save
    Result<void> load(const std::string& path);

    // Save to server.properties file
    Result<void> save(const std::string& path) const;

    // Populate with default values
    void set_defaults();

    // Get properties
    std::string get_string(const std::string& key, const std::string& default_value) const;
    i32 get_int(const std::string& key, i32 default_value) const;
    bool get_bool(const std::string& key, bool default_value) const;
    i64 get_long(const std::string& key, i64 default_value) const;

    // Set properties
    void set_string(const std::string& key, const std::string& value);
    void set_int(const std::string& key, i32 value);
    void set_bool(const std::string& key, bool value);
    void set_long(const std::string& key, i64 value);

    // Common properties
    std::string server_ip() const { return get_string("server-ip", ""); }
    u16 server_port() const { return static_cast<u16>(get_int("server-port", 25565)); }
    std::string level_name() const { return get_string("level-name", "world"); }
    std::string level_seed() const { return get_string("level-seed", ""); }
    bool online_mode() const { return get_bool("online-mode", true); }
    bool spawn_animals() const { return get_bool("spawn-animals", true); }
    bool spawn_monsters() const { return get_bool("spawn-monsters", true); }
    bool pvp() const { return get_bool("pvp", true); }
    bool allow_flight() const { return get_bool("allow-flight", false); }
    bool allow_nether() const { return get_bool("allow-nether", true); }
    i32 max_players() const { return get_int("max-players", 20); }

private:
    std::map<std::string, std::string> properties_;

    static std::string trim(const std::string& str);
};

} // namespace mcserver
