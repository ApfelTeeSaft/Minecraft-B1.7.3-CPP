/**
 * Example Plugin: HelloWorld
 *
 * Demonstrates the basic plugin API including:
 * - Plugin lifecycle (on_enable, on_disable)
 * - Event registration and handling
 * - Player events (join, quit, chat)
 * - Block events (place, break)
 */

#include "plugin/plugin.hpp"
#include "plugin/event/event_manager.hpp"
#include "plugin/event/player_events.hpp"
#include "plugin/event/block_events.hpp"
#include "util/log/logger.hpp"

using namespace mcserver;

class HelloWorldPlugin : public Plugin {
public:
    HelloWorldPlugin() {
        description_.name = "HelloWorld";
        description_.version = "1.0.0";
        description_.author = "ExampleAuthor";
        description_.description = "A simple example plugin demonstrating the plugin API";
    }

    void on_enable() override {
        LOG_INFO_CAT("HelloWorld plugin enabled!", LogCategory::Plugin);

        // Register event listeners
        register_event_listeners();
    }

    void on_disable() override {
        LOG_INFO_CAT("HelloWorld plugin disabled!", LogCategory::Plugin);

        // Event listeners are automatically unregistered by the plugin loader
    }

    const PluginDescription& get_description() const override {
        return description_;
    }

private:
    PluginDescription description_;

    void register_event_listeners() {
        auto* event_mgr = get_event_manager();
        if (!event_mgr) return;

        // Player join event - NORMAL priority
        event_mgr->register_listener<PlayerJoinEvent>(
            this,
            EventPriority::NORMAL,
            [this](PlayerJoinEvent& event) {
                handle_player_join(event);
            }
        );

        // Player quit event - NORMAL priority
        event_mgr->register_listener<PlayerQuitEvent>(
            this,
            EventPriority::NORMAL,
            [this](PlayerQuitEvent& event) {
                handle_player_quit(event);
            }
        );

        // Player chat event - HIGH priority (runs before other plugins)
        event_mgr->register_listener<PlayerChatEvent>(
            this,
            EventPriority::HIGH,
            [this](PlayerChatEvent& event) {
                handle_player_chat(event);
            }
        );

        // Block place event - NORMAL priority
        event_mgr->register_listener<BlockPlaceEvent>(
            this,
            EventPriority::NORMAL,
            [this](BlockPlaceEvent& event) {
                handle_block_place(event);
            },
            true  // Ignore cancelled events
        );

        // Block break event - NORMAL priority
        event_mgr->register_listener<BlockBreakEvent>(
            this,
            EventPriority::NORMAL,
            [this](BlockBreakEvent& event) {
                handle_block_break(event);
            },
            true  // Ignore cancelled events
        );

        LOG_INFO_CAT("Registered event listeners for HelloWorld plugin", LogCategory::Plugin);
    }

    void handle_player_join(PlayerJoinEvent& event) {
        (void)event;
        LOG_INFO_CAT("Player joined the server! Welcome!", LogCategory::Plugin);

        // Could modify join message:
        // event.set_join_message("§e[+] Player joined the game!");
    }

    void handle_player_quit(PlayerQuitEvent& event) {
        (void)event;
        LOG_INFO_CAT("Player left the server. Goodbye!", LogCategory::Plugin);

        // Could modify quit message:
        // event.set_quit_message("§e[-] Player left the game!");
    }

    void handle_player_chat(PlayerChatEvent& event) {
        const std::string& message = event.get_message();

        // Check for custom commands
        if (message.starts_with("!hello")) {
            LOG_INFO_CAT("Player said hello! Responding...", LogCategory::Plugin);

            // Cancel the chat event and handle it ourselves
            event.set_cancelled(true);

            // In a proper implementation, you would send a message back to the player
            // player->send_message("Hello to you too!");
        } else if (message.starts_with("!help")) {
            LOG_INFO_CAT("Player requested help", LogCategory::Plugin);
            event.set_cancelled(true);

            // Send help message
            // player->send_message("§eAvailable commands:");
            // player->send_message("§a!hello - Say hello");
            // player->send_message("§a!help - Show this message");
        }
    }

    void handle_block_place(BlockPlaceEvent& event) {
        LOG_INFO_CAT("Block placed at (" +
                    std::to_string(event.get_x()) + ", " +
                    std::to_string(event.get_y()) + ", " +
                    std::to_string(event.get_z()) + ") - Type: " +
                    std::to_string(event.get_block_type()),
                    LogCategory::Plugin);

        // Example: Prevent placing bedrock (block ID 7)
        if (event.get_block_type() == 7) {
            LOG_INFO_CAT("Preventing bedrock placement!", LogCategory::Plugin);
            event.set_cancelled(true);
        }
    }

    void handle_block_break(BlockBreakEvent& event) {
        LOG_INFO_CAT("Block broken at (" +
                    std::to_string(event.get_x()) + ", " +
                    std::to_string(event.get_y()) + ", " +
                    std::to_string(event.get_z()) + ") - Type: " +
                    std::to_string(event.get_block_type()),
                    LogCategory::Plugin);

        // Example: Prevent breaking bedrock
        if (event.get_block_type() == 7) {
            LOG_INFO_CAT("Preventing bedrock breakage!", LogCategory::Plugin);
            event.set_cancelled(true);
        }
    }
};

// Plugin factory functions (required exports)
extern "C" {
    Plugin* create_plugin() {
        return new HelloWorldPlugin();
    }

    void destroy_plugin(Plugin* plugin) {
        delete plugin;
    }
}
