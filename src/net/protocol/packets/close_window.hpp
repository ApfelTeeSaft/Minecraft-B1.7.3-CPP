#pragma once

#include "net/protocol/packet.hpp"
#include "util/types.hpp"

namespace mcserver {

// Packet 101 - Close Window
// Sent when a player closes a window (inventory, chest, etc.)
// Server-bound only
class PacketCloseWindow : public Packet {
public:
    i8 window_id;  // 0 for player inventory

    PacketCloseWindow() : window_id(0) {}

    explicit PacketCloseWindow(i8 wid) : window_id(wid) {}

    PacketId get_id() const override {
        return PacketId::CloseWindow;
    }

    usize estimated_size() const override {
        return 1; // 1 byte window_id
    }

    Result<void> read(PacketBuffer& buffer) override {
        auto wid_result = buffer.read_i8();
        if (!wid_result) return wid_result.error();
        window_id = wid_result.value();
        return {};
    }

    Result<void> write(PacketBuffer& buffer) const override {
        buffer.write_i8(window_id);
        return {};
    }
};

} // namespace mcserver
