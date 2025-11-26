#pragma once

#include "net/protocol/packet.hpp"

namespace mcserver {

// Packet 12: PlayerLook
// Client -> Server
// Sent when player rotates (rotation only, no movement)
class PacketPlayerLook : public Packet {
public:
    PacketPlayerLook() = default;
    PacketPlayerLook(f32 yaw, f32 pitch, bool on_ground);

    PacketId get_id() const override { return PacketId::PlayerLook; }
    Result<void> read(PacketBuffer& buffer) override;
    Result<void> write(PacketBuffer& buffer) const override;
    usize estimated_size() const override { return 9; } // 4+4+1

    f32 yaw = 0.0f;
    f32 pitch = 0.0f;
    bool on_ground = false;
};

} // namespace mcserver
