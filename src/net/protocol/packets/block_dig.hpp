#pragma once

#include "net/protocol/packet.hpp"

namespace mcserver {

// Digging status values
enum class DigStatus : u8 {
    Started = 0,        // Started digging
    Cancelled = 1,      // Cancelled digging
    Finished = 2,       // Finished digging
    DropItemStack = 3,  // Drop item stack (Q key)
    DropItem = 4,       // Drop single item (Ctrl+Q)
    ShootArrow = 5      // Shoot arrow / finish eating
};

// Packet 14: BlockDig
// Client -> Server
// Sent when player digs/breaks a block
class PacketBlockDig : public Packet {
public:
    PacketBlockDig() = default;
    PacketBlockDig(DigStatus status, i32 x, i8 y, i32 z, i8 face);

    PacketId get_id() const override { return PacketId::BlockDig; }
    Result<void> read(PacketBuffer& buffer) override;
    Result<void> write(PacketBuffer& buffer) const override;
    usize estimated_size() const override { return 11; } // 1+4+1+4+1

    DigStatus status = DigStatus::Started;
    i32 x = 0;
    i8 y = 0;
    i32 z = 0;
    i8 face = 0;  // Face being hit (0-5: -Y, +Y, -Z, +Z, -X, +X)
};

} // namespace mcserver
