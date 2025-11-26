#pragma once

#include "net/protocol/packet.hpp"

namespace mcserver {

// Packet 11: PlayerPosition
// Client -> Server
// Sent when player moves (position only, no rotation)
class PacketPlayerPosition : public Packet {
public:
    PacketPlayerPosition() = default;
    PacketPlayerPosition(f64 x, f64 y, f64 stance, f64 z, bool on_ground);

    PacketId get_id() const override { return PacketId::PlayerPosition; }
    Result<void> read(PacketBuffer& buffer) override;
    Result<void> write(PacketBuffer& buffer) const override;
    usize estimated_size() const override { return 33; } // 8+8+8+8+1

    f64 x = 0.0;
    f64 y = 0.0;       // Feet position
    f64 stance = 0.0;  // Eye position (y + 1.62)
    f64 z = 0.0;
    bool on_ground = false;
};

} // namespace mcserver
