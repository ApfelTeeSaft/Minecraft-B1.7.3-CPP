#pragma once

#include "event.hpp"
#include "util/types.hpp"
#include <string>

namespace mcserver {

class Player;

// Base class for all player events
class PlayerEvent : public Event {
public:
    explicit PlayerEvent(Player* player) : player_(player) {}

    Player* get_player() const { return player_; }

protected:
    Player* player_;
};

// Player join event
class PlayerJoinEvent : public PlayerEvent {
public:
    explicit PlayerJoinEvent(Player* player, std::string join_message = "")
        : PlayerEvent(player), join_message_(std::move(join_message)) {}

    const char* get_event_name() const override { return "PlayerJoinEvent"; }

    const std::string& get_join_message() const { return join_message_; }
    void set_join_message(std::string message) { join_message_ = std::move(message); }

private:
    std::string join_message_;
};

// Player quit event
class PlayerQuitEvent : public PlayerEvent {
public:
    explicit PlayerQuitEvent(Player* player, std::string quit_message = "")
        : PlayerEvent(player), quit_message_(std::move(quit_message)) {}

    const char* get_event_name() const override { return "PlayerQuitEvent"; }

    const std::string& get_quit_message() const { return quit_message_; }
    void set_quit_message(std::string message) { quit_message_ = std::move(message); }

private:
    std::string quit_message_;
};

// Player chat event (cancellable)
class PlayerChatEvent : public PlayerEvent, public CancellableEvent {
public:
    PlayerChatEvent(Player* player, std::string message)
        : PlayerEvent(player), message_(std::move(message)) {}

    const char* get_event_name() const override { return "PlayerChatEvent"; }

    const std::string& get_message() const { return message_; }
    void set_message(std::string message) { message_ = std::move(message); }

    // Format for display (e.g., "<PlayerName> message")
    std::string format_;
    const std::string& get_format() const { return format_; }
    void set_format(std::string format) { format_ = std::move(format); }

private:
    std::string message_;
};

// Player move event (cancellable)
class PlayerMoveEvent : public PlayerEvent, public CancellableEvent {
public:
    PlayerMoveEvent(Player* player, f64 from_x, f64 from_y, f64 from_z,
                   f64 to_x, f64 to_y, f64 to_z)
        : PlayerEvent(player),
          from_x_(from_x), from_y_(from_y), from_z_(from_z),
          to_x_(to_x), to_y_(to_y), to_z_(to_z) {}

    const char* get_event_name() const override { return "PlayerMoveEvent"; }

    // From position
    f64 get_from_x() const { return from_x_; }
    f64 get_from_y() const { return from_y_; }
    f64 get_from_z() const { return from_z_; }

    // To position
    f64 get_to_x() const { return to_x_; }
    f64 get_to_y() const { return to_y_; }
    f64 get_to_z() const { return to_z_; }

    // Modify destination
    void set_to(f64 x, f64 y, f64 z) {
        to_x_ = x;
        to_y_ = y;
        to_z_ = z;
    }

private:
    f64 from_x_, from_y_, from_z_;
    f64 to_x_, to_y_, to_z_;
};

// Player interact event (cancellable)
class PlayerInteractEvent : public PlayerEvent, public CancellableEvent {
public:
    enum class Action {
        LEFT_CLICK_AIR,
        LEFT_CLICK_BLOCK,
        RIGHT_CLICK_AIR,
        RIGHT_CLICK_BLOCK
    };

    PlayerInteractEvent(Player* player, Action action, i32 x = 0, i8 y = 0, i32 z = 0)
        : PlayerEvent(player), action_(action), x_(x), y_(y), z_(z) {}

    const char* get_event_name() const override { return "PlayerInteractEvent"; }

    Action get_action() const { return action_; }

    // Block coordinates (only valid for block interactions)
    i32 get_block_x() const { return x_; }
    i8 get_block_y() const { return y_; }
    i32 get_block_z() const { return z_; }

private:
    Action action_;
    i32 x_;
    i8 y_;
    i32 z_;
};

// Player respawn event
class PlayerRespawnEvent : public PlayerEvent {
public:
    PlayerRespawnEvent(Player* player, f64 x, f64 y, f64 z)
        : PlayerEvent(player), respawn_x_(x), respawn_y_(y), respawn_z_(z) {}

    const char* get_event_name() const override { return "PlayerRespawnEvent"; }

    f64 get_respawn_x() const { return respawn_x_; }
    f64 get_respawn_y() const { return respawn_y_; }
    f64 get_respawn_z() const { return respawn_z_; }

    void set_respawn_location(f64 x, f64 y, f64 z) {
        respawn_x_ = x;
        respawn_y_ = y;
        respawn_z_ = z;
    }

private:
    f64 respawn_x_, respawn_y_, respawn_z_;
};

} // namespace mcserver
