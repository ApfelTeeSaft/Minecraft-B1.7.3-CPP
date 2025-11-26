#pragma once

#include "net/protocol/packet.hpp"

namespace mcserver {

// Packet 8: UpdateHealth
// Server -> Client
// Sent when player's health changes
class PacketUpdateHealth : public Packet {
public:
    PacketUpdateHealth() = default;
    explicit PacketUpdateHealth(i16 health);

    PacketId get_id() const override { return PacketId::UpdateHealth; }
    Result<void> read(PacketBuffer& buffer) override;
    Result<void> write(PacketBuffer& buffer) const override;
    usize estimated_size() const override { return 2; }

    i16 health = 20; // 20 = full health (10 hearts)
};

} // namespace mcserver
