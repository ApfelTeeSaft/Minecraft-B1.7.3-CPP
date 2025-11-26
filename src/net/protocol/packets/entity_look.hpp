#pragma once

#include "net/protocol/packet.hpp"

namespace mcserver {

// Packet 32: EntityLook
// Server -> Client
// Sent when an entity rotates
class PacketEntityLook : public Packet {
public:
    PacketEntityLook() = default;
    PacketEntityLook(i32 entity_id, i8 yaw, i8 pitch);

    PacketId get_id() const override { return PacketId::EntityLook; }
    Result<void> read(PacketBuffer& buffer) override;
    Result<void> write(PacketBuffer& buffer) const override;
    usize estimated_size() const override { return 6; } // 4 + 1 + 1

    i32 entity_id = 0;
    i8 yaw = 0;    // Rotation: angle * 256 / 360
    i8 pitch = 0;
};

} // namespace mcserver
