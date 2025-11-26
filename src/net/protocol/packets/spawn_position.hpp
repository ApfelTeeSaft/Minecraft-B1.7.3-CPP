#pragma once

#include "net/protocol/packet.hpp"

namespace mcserver {

// Packet 6: SpawnPosition
class PacketSpawnPosition : public Packet {
public:
    PacketSpawnPosition() = default;
    PacketSpawnPosition(i32 x, i32 y, i32 z);

    PacketId get_id() const override { return PacketId::SpawnPosition; }
    Result<void> read(PacketBuffer& buffer) override;
    Result<void> write(PacketBuffer& buffer) const override;
    usize estimated_size() const override { return 12; }

    i32 x = 0;
    i32 y = 0;
    i32 z = 0;
};

} // namespace mcserver
