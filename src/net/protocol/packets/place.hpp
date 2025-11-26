#pragma once

#include "net/protocol/packet.hpp"

namespace mcserver {

// Packet 15: Place
// Client -> Server
// Sent when player places a block or uses an item
class PacketPlace : public Packet {
public:
    PacketPlace() = default;
    PacketPlace(i32 x, i8 y, i32 z, i8 direction, i16 block_item_id, u8 amount, i16 damage);

    PacketId get_id() const override { return PacketId::Place; }
    Result<void> read(PacketBuffer& buffer) override;
    Result<void> write(PacketBuffer& buffer) const override;
    usize estimated_size() const override { return 12; } // 4+1+4+1+2 (or more with item)

    i32 x = -1;      // -1 for special items
    i8 y = -1;       // -1 for special items (represents 255 in unsigned)
    i32 z = -1;      // -1 for special items
    i8 direction = 0; // Face clicked (0-5: -Y, +Y, -Z, +Z, -X, +X)
    i16 block_item_id = -1;  // Item being held (-1 for empty hand)
    u8 amount = 0;    // Only sent if block_item_id != -1
    i16 damage = 0;   // Only sent if block_item_id != -1
};

} // namespace mcserver
