#pragma once

#include "net/protocol/packet.hpp"
#include "util/types.hpp"

namespace mcserver {

// Packet 7 - UseEntity
// Client -> Server
// Sent when a player attacks or interacts with an entity
class PacketUseEntity : public Packet {
public:
    i32 user_id;      // The player performing the action
    i32 target_id;    // The entity being targeted
    bool left_click;  // true = attack (left click), false = interact (right click)

    PacketUseEntity() : user_id(0), target_id(0), left_click(false) {}

    PacketUseEntity(i32 user, i32 target, bool left)
        : user_id(user), target_id(target), left_click(left) {}

    PacketId get_id() const override {
        return PacketId::UseEntity;
    }

    usize estimated_size() const override {
        return 9; // 4 bytes user_id + 4 bytes target_id + 1 byte left_click
    }

    Result<void> read(PacketBuffer& buffer) override {
        auto user_result = buffer.read_i32();
        if (!user_result) return user_result.error();
        user_id = user_result.value();

        auto target_result = buffer.read_i32();
        if (!target_result) return target_result.error();
        target_id = target_result.value();

        auto click_result = buffer.read_bool();
        if (!click_result) return click_result.error();
        left_click = click_result.value();

        return {};
    }

    Result<void> write(PacketBuffer& buffer) const override {
        buffer.write_i32(user_id);
        buffer.write_i32(target_id);
        buffer.write_bool(left_click);
        return {};
    }
};

} // namespace mcserver
