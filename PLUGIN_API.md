# Plugin API Documentation

## Overview

The Minecraft Beta 1.7.3 Server implements a Bukkit-inspired plugin system that allows you to extend server functionality through dynamically loaded plugins written in C++.

## Features

- **Event System**: Register listeners for server events with configurable priorities
- **Cancellable Events**: Prevent default behavior by cancelling events
- **Dynamic Loading**: Load and unload plugins at runtime
- **Lifecycle Management**: Proper `on_enable()` and `on_disable()` hooks
- **Type-Safe**: Full C++ type safety with compile-time checking
- **Cross-Platform**: Works on Linux, macOS, and Windows

## Plugin Structure

### Basic Plugin Class

```cpp
#include "plugin/plugin.hpp"
#include "plugin/event/event_manager.hpp"

class MyPlugin : public mcserver::Plugin {
public:
    MyPlugin() {
        description_.name = "MyPlugin";
        description_.version = "1.0.0";
        description_.author = "YourName";
        description_.description = "My awesome plugin";
    }

    void on_enable() override {
        // Called when plugin is enabled
        // Register event listeners here
    }

    void on_disable() override {
        // Called when plugin is disabled
        // Cleanup code here
    }

    const mcserver::PluginDescription& get_description() const override {
        return description_;
    }

private:
    mcserver::PluginDescription description_;
};

// Required exports
extern "C" {
    mcserver::Plugin* create_plugin() {
        return new MyPlugin();
    }

    void destroy_plugin(mcserver::Plugin* plugin) {
        delete plugin;
    }
}
```

## Event System

### Event Priorities

Events are dispatched in order of priority:

- `LOWEST` - Runs first
- `LOW`
- `NORMAL` - Default priority
- `HIGH`
- `HIGHEST`
- `MONITOR` - Runs last, for monitoring only (should not modify event)

### Registering Event Listeners

```cpp
void on_enable() override {
    auto* event_mgr = get_event_manager();

    event_mgr->register_listener<PlayerJoinEvent>(
        this,                        // Plugin pointer
        EventPriority::NORMAL,       // Priority
        [this](PlayerJoinEvent& event) {
            // Handle event
        },
        false                        // ignore_cancelled (optional)
    );
}
```

### Cancellable Events

Some events can be cancelled to prevent their default behavior:

```cpp
event_mgr->register_listener<BlockPlaceEvent>(
    this,
    EventPriority::HIGH,
    [](BlockPlaceEvent& event) {
        if (event.get_block_type() == 7) {  // Bedrock
            event.set_cancelled(true);       // Prevent placement
        }
    }
);
```

## Available Events

### Player Events (`player_events.hpp`)

- **PlayerJoinEvent**: When a player joins the server
  - Methods: `get_player()`, `get/set_join_message()`
  - Not cancellable

- **PlayerQuitEvent**: When a player leaves the server
  - Methods: `get_player()`, `get/set_quit_message()`
  - Not cancellable

- **PlayerChatEvent**: When a player sends a chat message
  - Methods: `get_player()`, `get/set_message()`, `get/set_format()`
  - Cancellable

- **PlayerMoveEvent**: When a player moves
  - Methods: `get_player()`, `get_from_*()`, `get_to_*()`, `set_to()`
  - Cancellable

- **PlayerInteractEvent**: When a player interacts with world
  - Methods: `get_player()`, `get_action()`, `get_block_*()`
  - Actions: `LEFT_CLICK_AIR`, `LEFT_CLICK_BLOCK`, `RIGHT_CLICK_AIR`, `RIGHT_CLICK_BLOCK`
  - Cancellable

- **PlayerRespawnEvent**: When a player respawns
  - Methods: `get_player()`, `get/set_respawn_location()`
  - Not cancellable

### Block Events (`block_events.hpp`)

- **BlockPlaceEvent**: When a block is placed
  - Methods: `get_x/y/z()`, `get/set_block_type()`, `get/set_metadata()`, `get_player()`
  - Cancellable

- **BlockBreakEvent**: When a block is broken
  - Methods: `get_x/y/z()`, `get_block_type()`, `get_player()`, `should_drop_items()`, `set_drop_items()`
  - Cancellable

- **BlockInteractEvent**: When a player interacts with a block
  - Methods: `get_x/y/z()`, `get_block_type()`, `get_player()`
  - Cancellable

### Entity Events (`entity_events.hpp`)

- **EntitySpawnEvent**: When an entity spawns
  - Methods: `get_entity()`
  - Cancellable

- **EntityDeathEvent**: When an entity dies
  - Methods: `get_entity()`, `get_killer()`, `get/set_dropped_exp()`, `should_drop_items()`
  - Not cancellable

- **EntityDamageEvent**: When an entity takes damage
  - Methods: `get_entity()`, `get_cause()`, `get/set_damage()`
  - Damage causes: `CONTACT`, `ENTITY_ATTACK`, `PROJECTILE`, `FALL`, `FIRE`, `LAVA`, `DROWNING`, etc.
  - Cancellable

- **EntityDamageByEntityEvent**: When an entity damages another entity
  - Methods: `get_entity()`, `get_damager()`, `get/set_damage()`
  - Cancellable

- **EntityTargetEvent**: When a mob targets something
  - Methods: `get_entity()`, `get/set_target()`, `get_reason()`
  - Reasons: `TARGET_ATTACKED_ENTITY`, `CLOSEST_PLAYER`, `FORGOT_TARGET`, etc.
  - Cancellable

### Server Events (`server_events.hpp`)

- **ServerEnableEvent**: When server starts
  - Methods: `get_server()`
  - Not cancellable

- **ServerDisableEvent**: When server stops
  - Methods: `get_server()`
  - Not cancellable

- **ChunkLoadEvent**: When a chunk is loaded
  - Methods: `get_chunk_x/z()`, `is_new_chunk()`
  - Not cancellable

- **ChunkUnloadEvent**: When a chunk is unloaded
  - Methods: `get_chunk_x/z()`
  - Not cancellable

## Building Plugins

### Requirements

- C++23 compiler (or C++20 minimum)
- CMake 3.23 or higher
- Access to server headers

### CMake Example

```cmake
cmake_minimum_required(VERSION 3.23)

project(MyPlugin VERSION 1.0.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

add_library(my_plugin SHARED
    my_plugin.cpp
)

target_include_directories(my_plugin PRIVATE
    /path/to/server/src
)

set_target_properties(my_plugin PROPERTIES
    PREFIX ""  # Remove "lib" prefix
)
```

### Build Commands

```bash
mkdir build && cd build
cmake ..
cmake --build .
```

This will produce `my_plugin.so` (Linux), `my_plugin.dylib` (macOS), or `my_plugin.dll` (Windows).

## Loading Plugins

### Plugin Directory Structure

```
server_root/
├── mcserver (executable)
└── plugins/
    ├── my_plugin.so
    ├── another_plugin.so
    └── ...
```

### Automatic Loading

Plugins in the `plugins/` directory are automatically loaded on server start.

### Manual Loading (via API)

```cpp
PluginLoader loader(&server, &event_manager);

// Load single plugin
auto result = loader.load_plugin("plugins/my_plugin.so");
if (result.is_ok()) {
    Plugin* plugin = result.value();
    loader.enable_plugin(plugin->get_description().name);
}

// Load all plugins from directory
loader.load_plugins_from_directory("plugins/");
loader.enable_all_plugins();
```

## Best Practices

### 1. Event Listener Management

Always use `ignore_cancelled = true` for MONITOR priority listeners:

```cpp
event_mgr->register_listener<BlockPlaceEvent>(
    this,
    EventPriority::MONITOR,
    [](BlockPlaceEvent& event) {
        // Just log, don't modify
        log_block_placement(event);
    },
    true  // Ignore cancelled events
);
```

### 2. Error Handling

Plugins should handle errors internally since exceptions are disabled:

```cpp
void on_enable() override {
    if (!initialize_database()) {
        LOG_ERROR_CAT("Failed to initialize database!", LogCategory::Plugin);
        return;
    }
    // Continue initialization...
}
```

### 3. Resource Cleanup

Always clean up resources in `on_disable()`:

```cpp
void on_disable() override {
    // Save data
    save_plugin_data();

    // Close connections
    database_connection_.close();

    // Event listeners are automatically unregistered
}
```

### 4. Thread Safety

Event handlers may be called from multiple threads. Use mutexes for shared data:

```cpp
class MyPlugin : public Plugin {
private:
    std::mutex mutex_;
    std::map<std::string, PlayerData> player_data_;

    void handle_player_join(PlayerJoinEvent& event) {
        std::lock_guard<std::mutex> lock(mutex_);
        player_data_[event.get_player()->get_name()] = PlayerData();
    }
};
```

## Example Plugin

See `examples/plugins/hello_world/` for a complete example plugin demonstrating:

- Plugin lifecycle management
- Event registration and handling
- Event cancellation
- Custom commands via chat events
- Block placement/breaking restrictions

## API Reference

### Plugin Interface

- `void on_enable()` - Called when plugin is enabled
- `void on_disable()` - Called when plugin is disabled
- `const PluginDescription& get_description() const` - Returns plugin metadata
- `Server* get_server() const` - Get server instance
- `EventManager* get_event_manager() const` - Get event manager
- `bool is_enabled() const` - Check if plugin is enabled

### EventManager

- `register_listener<EventType>(plugin, priority, handler, ignore_cancelled)` - Register event listener
- `call_event<EventType>(event)` - Dispatch event to listeners
- `unregister_plugin(plugin)` - Unregister all listeners for a plugin
- `unregister_all()` - Unregister all listeners
- `get_listener_count()` - Get total number of registered listeners

### PluginLoader

- `load_plugin(file_path)` - Load a single plugin
- `load_plugins_from_directory(directory)` - Load all plugins from directory
- `unload_plugin(plugin_name)` - Unload a plugin
- `enable_plugin(plugin_name)` - Enable a loaded plugin
- `disable_plugin(plugin_name)` - Disable a plugin
- `get_plugin(plugin_name)` - Get plugin by name
- `get_plugins()` - Get all loaded plugins
- `is_plugin_loaded(plugin_name)` - Check if plugin is loaded

## Troubleshooting

### Plugin Not Loading

1. **Check file extension**: Must be `.so` (Linux), `.dylib` (macOS), or `.dll` (Windows)
2. **Verify exports**: Ensure `create_plugin` and `destroy_plugin` are exported with `extern "C"`
3. **Check logs**: Look for error messages in server logs under `[Plugin]` category
4. **Dependencies**: Ensure all dependencies are available

### Events Not Firing

1. **Registration**: Verify event listener is registered in `on_enable()`
2. **Priority**: Check if higher priority listeners are cancelling the event
3. **ignore_cancelled**: If set to `true`, cancelled events won't reach your handler
4. **Event type**: Ensure you're using the correct event type

### Crash on Load/Unload

1. **Memory management**: Ensure proper cleanup in `on_disable()`
2. **Static variables**: Avoid global/static variables that aren't cleaned up
3. **Threading**: Use proper synchronization for shared data
4. **ABI compatibility**: Ensure plugin is built with same compiler/settings as server

## Contributing

To add new events:

1. Create event class inheriting from `Event` or `CancellableEvent`
2. Add to appropriate header in `src/plugin/event/`
3. Call `event_manager.call_event()` at the appropriate point in server code
4. Update this documentation

## License

See LICENSE file in repository root.
