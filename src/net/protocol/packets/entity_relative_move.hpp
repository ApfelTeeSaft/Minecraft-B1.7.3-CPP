#pragma once

#include "net/protocol/packet.hpp"

namespace mcserver {

// Packet 31: EntityRelativeMove
// Server -> Client
// Sent when an entity moves by a small amount (< 4 blocks)
class PacketEntityRelativeMove : public Packet {
public:
    PacketEntityRelativeMove() = default;
    PacketEntityRelativeMove(i32 entity_id, i8 dx, i8 dy, i8 dz);

    PacketId get_id() const override { return PacketId::RelEntityMove; }
    Result<void> read(PacketBuffer& buffer) override;
    Result<void> write(PacketBuffer& buffer) const override;
    usize estimated_size() const override { return 7; } // 4 + 1 + 1 + 1

    i32 entity_id = 0;
    i8 dx = 0;  // Relative movement in fixed-point (actual * 32)
    i8 dy = 0;
    i8 dz = 0;
};

} // namespace mcserver
