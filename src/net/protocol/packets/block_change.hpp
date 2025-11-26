#pragma once

#include "net/protocol/packet.hpp"

namespace mcserver {

// Packet 53: BlockChange
// Server -> Client
// Sent when a single block is changed
class PacketBlockChange : public Packet {
public:
    PacketBlockChange() = default;
    PacketBlockChange(i32 x, i8 y, i32 z, u8 block_type, u8 block_metadata);

    PacketId get_id() const override { return PacketId::BlockChange; }
    Result<void> read(PacketBuffer& buffer) override;
    Result<void> write(PacketBuffer& buffer) const override;
    usize estimated_size() const override { return 11; } // 4+1+4+1+1

    i32 x = 0;
    i8 y = 0;
    i32 z = 0;
    u8 block_type = 0;      // Block ID (0 = air)
    u8 block_metadata = 0;  // Block data/metadata
};

} // namespace mcserver
