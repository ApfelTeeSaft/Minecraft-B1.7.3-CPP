#pragma once

#include "net/protocol/packet.hpp"

namespace mcserver {

// Packet 13: PlayerPositionLook
class PacketPlayerPositionLook : public Packet {
public:
    PacketPlayerPositionLook() = default;
    PacketPlayerPositionLook(f64 x, f64 y, f64 stance, f64 z, f32 yaw, f32 pitch, bool on_ground);

    PacketId get_id() const override { return PacketId::PlayerLookMove; }
    Result<void> read(PacketBuffer& buffer) override;
    Result<void> write(PacketBuffer& buffer) const override;
    usize estimated_size() const override { return 41; }

    f64 x = 0.0;
    f64 y = 0.0;
    f64 stance = 0.0; // Player eye height
    f64 z = 0.0;
    f32 yaw = 0.0f;
    f32 pitch = 0.0f;
    bool on_ground = false;
};

} // namespace mcserver
