#pragma once

#include "net/protocol/packet.hpp"

namespace mcserver {

// Packet 4: UpdateTime
class PacketUpdateTime : public Packet {
public:
    PacketUpdateTime() = default;
    explicit PacketUpdateTime(i64 time);

    PacketId get_id() const override { return PacketId::UpdateTime; }
    Result<void> read(PacketBuffer& buffer) override;
    Result<void> write(PacketBuffer& buffer) const override;
    usize estimated_size() const override { return 8; }

    i64 time = 0;
};

} // namespace mcserver
