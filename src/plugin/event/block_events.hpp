#pragma once

#include "event.hpp"
#include "util/types.hpp"

namespace mcserver {

class Player;

// Base class for all block events
class BlockEvent : public Event {
public:
    BlockEvent(i32 x, i8 y, i32 z) : x_(x), y_(y), z_(z) {}

    i32 get_x() const { return x_; }
    i8 get_y() const { return y_; }
    i32 get_z() const { return z_; }

protected:
    i32 x_;
    i8 y_;
    i32 z_;
};

// Block place event (cancellable)
class BlockPlaceEvent : public BlockEvent, public CancellableEvent {
public:
    BlockPlaceEvent(i32 x, i8 y, i32 z, u8 block_type, u8 metadata, Player* player)
        : BlockEvent(x, y, z), block_type_(block_type), metadata_(metadata), player_(player) {}

    const char* get_event_name() const override { return "BlockPlaceEvent"; }

    u8 get_block_type() const { return block_type_; }
    u8 get_metadata() const { return metadata_; }
    Player* get_player() const { return player_; }

    void set_block_type(u8 type) { block_type_ = type; }
    void set_metadata(u8 meta) { metadata_ = meta; }

private:
    u8 block_type_;
    u8 metadata_;
    Player* player_;
};

// Block break event (cancellable)
class BlockBreakEvent : public BlockEvent, public CancellableEvent {
public:
    BlockBreakEvent(i32 x, i8 y, i32 z, u8 block_type, Player* player)
        : BlockEvent(x, y, z), block_type_(block_type), player_(player) {}

    const char* get_event_name() const override { return "BlockBreakEvent"; }

    u8 get_block_type() const { return block_type_; }
    Player* get_player() const { return player_; }

    // Control whether drops should be given
    bool should_drop_items() const { return drop_items_; }
    void set_drop_items(bool drop) { drop_items_ = drop; }

private:
    u8 block_type_;
    Player* player_;
    bool drop_items_ = true;
};

// Block interact event (cancellable) - for things like buttons, doors, etc.
class BlockInteractEvent : public BlockEvent, public CancellableEvent {
public:
    BlockInteractEvent(i32 x, i8 y, i32 z, u8 block_type, Player* player)
        : BlockEvent(x, y, z), block_type_(block_type), player_(player) {}

    const char* get_event_name() const override { return "BlockInteractEvent"; }

    u8 get_block_type() const { return block_type_; }
    Player* get_player() const { return player_; }

private:
    u8 block_type_;
    Player* player_;
};

} // namespace mcserver
