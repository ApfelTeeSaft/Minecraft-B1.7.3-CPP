#pragma once

#include "plugin/plugin.hpp"
#include "plugin/event/event_manager.hpp"
#include "util/types.hpp"
#include "util/result.hpp"
#include <string>
#include <vector>
#include <memory>
#include <map>

namespace mcserver {

class Server;

// Loaded plugin information
struct LoadedPlugin {
    std::unique_ptr<Plugin> plugin;
    void* library_handle;  // Platform-specific (dlopen handle on Unix, HMODULE on Windows)
    std::string file_path;
    PluginDestructor destructor;

    LoadedPlugin(std::unique_ptr<Plugin> p, void* handle, std::string path, PluginDestructor dest)
        : plugin(std::move(p)), library_handle(handle), file_path(std::move(path)), destructor(dest) {}
};

// Plugin loader - loads plugins from shared libraries
class PluginLoader {
public:
    explicit PluginLoader(Server* server, EventManager* event_manager);
    ~PluginLoader();

    // Load a single plugin from a shared library file
    Result<Plugin*> load_plugin(const std::string& file_path);

    // Load all plugins from a directory
    Result<usize> load_plugins_from_directory(const std::string& directory);

    // Unload a specific plugin by name
    Result<void> unload_plugin(const std::string& plugin_name);

    // Unload all plugins
    void unload_all_plugins();

    // Enable a loaded plugin (calls on_enable)
    Result<void> enable_plugin(const std::string& plugin_name);

    // Disable a loaded plugin (calls on_disable)
    Result<void> disable_plugin(const std::string& plugin_name);

    // Enable all loaded plugins
    void enable_all_plugins();

    // Disable all loaded plugins
    void disable_all_plugins();

    // Get a loaded plugin by name
    Plugin* get_plugin(const std::string& plugin_name) const;

    // Get all loaded plugins
    std::vector<Plugin*> get_plugins() const;

    // Check if a plugin is loaded
    bool is_plugin_loaded(const std::string& plugin_name) const;

    // Get plugin count
    usize get_plugin_count() const { return plugins_.size(); }

private:
    Server* server_;
    EventManager* event_manager_;
    std::map<std::string, LoadedPlugin> plugins_;

    // Platform-specific library loading
    void* load_library(const std::string& file_path);
    void unload_library(void* handle);
    void* get_symbol(void* handle, const std::string& symbol_name);

    // Get platform-specific library extension
    static std::string get_library_extension();
};

} // namespace mcserver
