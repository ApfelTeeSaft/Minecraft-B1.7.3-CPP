#pragma once

#include "util/types.hpp"
#include <string>
#include <memory>

namespace mcserver {

class Server;
class EventManager;

// Plugin metadata
struct PluginDescription {
    std::string name;
    std::string version;
    std::string author;
    std::string description;
    std::string main_class;  // For C++ plugins, unused but kept for compatibility

    PluginDescription() = default;

    PluginDescription(std::string n, std::string v, std::string a = "", std::string d = "")
        : name(std::move(n)), version(std::move(v)), author(std::move(a)), description(std::move(d)) {}
};

// Base class for all plugins
class Plugin {
public:
    virtual ~Plugin() = default;

    // Plugin lifecycle methods (similar to Bukkit)
    virtual void on_enable() = 0;
    virtual void on_disable() = 0;

    // Get plugin metadata
    virtual const PluginDescription& get_description() const = 0;

    // Get server instance
    Server* get_server() const { return server_; }

    // Get event manager
    EventManager* get_event_manager() const { return event_manager_; }

    // Check if plugin is enabled
    bool is_enabled() const { return enabled_; }

    // Internal: Set server instance (called by plugin loader)
    void set_server(Server* server) { server_ = server; }

    // Internal: Set event manager (called by plugin loader)
    void set_event_manager(EventManager* event_manager) { event_manager_ = event_manager; }

    // Internal: Set enabled state (called by plugin loader)
    void set_enabled(bool enabled) { enabled_ = enabled; }

protected:
    Server* server_ = nullptr;
    EventManager* event_manager_ = nullptr;
    bool enabled_ = false;
};

// Plugin factory function type
// Each plugin shared library must export a function with this signature
using PluginFactory = Plugin* (*)();

// Plugin destructor function type
using PluginDestructor = void (*)(Plugin*);

} // namespace mcserver
