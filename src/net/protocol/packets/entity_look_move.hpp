#pragma once

#include "net/protocol/packet.hpp"

namespace mcserver {

// Packet 33: RelEntityMoveLook
// Server -> Client
// Sent when an entity moves and rotates at the same time
class PacketEntityLookMove : public Packet {
public:
    PacketEntityLookMove() = default;
    PacketEntityLookMove(i32 entity_id, i8 dx, i8 dy, i8 dz, i8 yaw, i8 pitch);

    PacketId get_id() const override { return PacketId::RelEntityMoveLook; }
    Result<void> read(PacketBuffer& buffer) override;
    Result<void> write(PacketBuffer& buffer) const override;
    usize estimated_size() const override { return 9; } // 4 + 1 + 1 + 1 + 1 + 1

    i32 entity_id = 0;
    i8 dx = 0;     // Relative movement in fixed-point (actual * 32)
    i8 dy = 0;
    i8 dz = 0;
    i8 yaw = 0;    // Rotation: angle * 256 / 360
    i8 pitch = 0;
};

} // namespace mcserver
