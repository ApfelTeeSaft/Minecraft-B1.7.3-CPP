#include "plugin_loader.hpp"
#include "util/log/logger.hpp"
#include <filesystem>
#include <algorithm>

// Platform-specific includes
#ifdef _WIN32
    #include <windows.h>
#else
    #include <dlfcn.h>
#endif

namespace mcserver {

PluginLoader::PluginLoader(Server* server, EventManager* event_manager)
    : server_(server), event_manager_(event_manager) {
}

PluginLoader::~PluginLoader() {
    unload_all_plugins();
}

Result<Plugin*> PluginLoader::load_plugin(const std::string& file_path) {
    // Check if file exists
    if (!std::filesystem::exists(file_path)) {
        LOG_ERROR_CAT("Plugin file not found: " + file_path, LogCategory::Plugin);
        return Result<Plugin*>(ErrorCode::NotFound);
    }

    // Load the shared library
    void* handle = load_library(file_path);
    if (!handle) {
        LOG_ERROR_CAT("Failed to load plugin library: " + file_path, LogCategory::Plugin);
        return Result<Plugin*>(ErrorCode::IOError);
    }

    // Get the plugin factory function
    auto create_plugin = reinterpret_cast<PluginFactory>(get_symbol(handle, "create_plugin"));
    if (!create_plugin) {
        LOG_ERROR_CAT("Plugin missing 'create_plugin' function: " + file_path, LogCategory::Plugin);
        unload_library(handle);
        return Result<Plugin*>(ErrorCode::InvalidArgument);
    }

    // Get the plugin destructor function
    auto destroy_plugin = reinterpret_cast<PluginDestructor>(get_symbol(handle, "destroy_plugin"));
    if (!destroy_plugin) {
        LOG_ERROR_CAT("Plugin missing 'destroy_plugin' function: " + file_path, LogCategory::Plugin);
        unload_library(handle);
        return Result<Plugin*>(ErrorCode::InvalidArgument);
    }

    // Create the plugin instance
    Plugin* plugin_ptr = create_plugin();
    if (!plugin_ptr) {
        LOG_ERROR_CAT("Failed to create plugin instance: " + file_path, LogCategory::Plugin);
        unload_library(handle);
        return Result<Plugin*>(ErrorCode::Unknown);
    }

    // Set up the plugin
    plugin_ptr->set_server(server_);
    plugin_ptr->set_event_manager(event_manager_);

    // Get plugin name
    std::string plugin_name = plugin_ptr->get_description().name;

    // Check for duplicate plugin names
    if (is_plugin_loaded(plugin_name)) {
        LOG_WARNING_CAT("Plugin already loaded: " + plugin_name, LogCategory::Plugin);
        destroy_plugin(plugin_ptr);
        unload_library(handle);
        return Result<Plugin*>(ErrorCode::AlreadyExists);
    }

    // Store the loaded plugin
    plugins_.emplace(plugin_name, LoadedPlugin(
        std::unique_ptr<Plugin>(plugin_ptr),
        handle,
        file_path,
        destroy_plugin
    ));

    LOG_INFO_CAT("Loaded plugin: " + plugin_name + " v" +
                 plugin_ptr->get_description().version, LogCategory::Plugin);

    return Result<Plugin*>(plugin_ptr);
}

Result<usize> PluginLoader::load_plugins_from_directory(const std::string& directory) {
    if (!std::filesystem::exists(directory)) {
        LOG_WARNING_CAT("Plugins directory not found: " + directory, LogCategory::Plugin);
        return Result<usize>(0);
    }

    if (!std::filesystem::is_directory(directory)) {
        LOG_ERROR_CAT("Path is not a directory: " + directory, LogCategory::Plugin);
        return Result<usize>(ErrorCode::InvalidArgument);
    }

    usize loaded_count = 0;
    std::string extension = get_library_extension();

    for (const auto& entry : std::filesystem::directory_iterator(directory)) {
        if (!entry.is_regular_file()) {
            continue;
        }

        std::string path = entry.path().string();
        if (path.ends_with(extension)) {
            auto result = load_plugin(path);
            if (result.is_ok()) {
                loaded_count++;
            }
        }
    }

    LOG_INFO_CAT("Loaded " + std::to_string(loaded_count) + " plugin(s) from: " + directory,
                 LogCategory::Plugin);

    return Result<usize>(loaded_count);
}

Result<void> PluginLoader::unload_plugin(const std::string& plugin_name) {
    auto it = plugins_.find(plugin_name);
    if (it == plugins_.end()) {
        return Result<void>(ErrorCode::NotFound);
    }

    auto& loaded_plugin = it->second;

    // Disable plugin if enabled
    if (loaded_plugin.plugin->is_enabled()) {
        auto result = disable_plugin(plugin_name);
        if (result.is_error()) {
            LOG_WARNING_CAT("Failed to disable plugin before unload: " + plugin_name, LogCategory::Plugin);
        }
    }

    // Unregister all event listeners
    event_manager_->unregister_plugin(loaded_plugin.plugin.get());

    // Destroy plugin instance
    if (loaded_plugin.destructor) {
        loaded_plugin.destructor(loaded_plugin.plugin.release());
    }

    // Unload library
    if (loaded_plugin.library_handle) {
        unload_library(loaded_plugin.library_handle);
    }

    LOG_INFO_CAT("Unloaded plugin: " + plugin_name, LogCategory::Plugin);

    plugins_.erase(it);
    return Result<void>();
}

void PluginLoader::unload_all_plugins() {
    // Disable all plugins first
    disable_all_plugins();

    // Unload in reverse order
    auto plugin_names = std::vector<std::string>();
    for (const auto& [name, _] : plugins_) {
        plugin_names.push_back(name);
    }

    std::reverse(plugin_names.begin(), plugin_names.end());

    for (const auto& name : plugin_names) {
        unload_plugin(name);
    }
}

Result<void> PluginLoader::enable_plugin(const std::string& plugin_name) {
    auto it = plugins_.find(plugin_name);
    if (it == plugins_.end()) {
        return Result<void>(ErrorCode::NotFound);
    }

    auto& plugin = it->second.plugin;

    if (plugin->is_enabled()) {
        return Result<void>();  // Already enabled
    }

    // Call on_enable (plugins should handle errors internally)
    plugin->on_enable();
    plugin->set_enabled(true);
    LOG_INFO_CAT("Enabled plugin: " + plugin_name, LogCategory::Plugin);

    return Result<void>();
}

Result<void> PluginLoader::disable_plugin(const std::string& plugin_name) {
    auto it = plugins_.find(plugin_name);
    if (it == plugins_.end()) {
        return Result<void>(ErrorCode::NotFound);
    }

    auto& plugin = it->second.plugin;

    if (!plugin->is_enabled()) {
        return Result<void>();  // Already disabled
    }

    // Call on_disable (plugins should handle errors internally)
    plugin->on_disable();
    plugin->set_enabled(false);
    LOG_INFO_CAT("Disabled plugin: " + plugin_name, LogCategory::Plugin);

    return Result<void>();
}

void PluginLoader::enable_all_plugins() {
    for (const auto& [name, _] : plugins_) {
        enable_plugin(name);
    }
}

void PluginLoader::disable_all_plugins() {
    // Disable in reverse order
    auto plugin_names = std::vector<std::string>();
    for (const auto& [name, _] : plugins_) {
        plugin_names.push_back(name);
    }

    std::reverse(plugin_names.begin(), plugin_names.end());

    for (const auto& name : plugin_names) {
        disable_plugin(name);
    }
}

Plugin* PluginLoader::get_plugin(const std::string& plugin_name) const {
    auto it = plugins_.find(plugin_name);
    if (it == plugins_.end()) {
        return nullptr;
    }
    return it->second.plugin.get();
}

std::vector<Plugin*> PluginLoader::get_plugins() const {
    std::vector<Plugin*> result;
    result.reserve(plugins_.size());

    for (const auto& [name, loaded] : plugins_) {
        result.push_back(loaded.plugin.get());
    }

    return result;
}

bool PluginLoader::is_plugin_loaded(const std::string& plugin_name) const {
    return plugins_.find(plugin_name) != plugins_.end();
}

// Platform-specific implementations

void* PluginLoader::load_library(const std::string& file_path) {
#ifdef _WIN32
    return LoadLibraryA(file_path.c_str());
#else
    return dlopen(file_path.c_str(), RTLD_LAZY);
#endif
}

void PluginLoader::unload_library(void* handle) {
    if (!handle) return;

#ifdef _WIN32
    FreeLibrary(static_cast<HMODULE>(handle));
#else
    dlclose(handle);
#endif
}

void* PluginLoader::get_symbol(void* handle, const std::string& symbol_name) {
    if (!handle) return nullptr;

#ifdef _WIN32
    return reinterpret_cast<void*>(GetProcAddress(static_cast<HMODULE>(handle), symbol_name.c_str()));
#else
    return dlsym(handle, symbol_name.c_str());
#endif
}

std::string PluginLoader::get_library_extension() {
#ifdef _WIN32
    return ".dll";
#elif defined(__APPLE__)
    return ".dylib";
#else
    return ".so";
#endif
}

} // namespace mcserver
