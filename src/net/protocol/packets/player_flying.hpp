#pragma once

#include "net/protocol/packet.hpp"

namespace mcserver {

// Packet 10: Flying
// Client -> Server
// Sent when player is on ground state changes
class PacketPlayerFlying : public Packet {
public:
    PacketPlayerFlying() = default;
    explicit PacketPlayerFlying(bool on_ground);

    PacketId get_id() const override { return PacketId::Flying; }
    Result<void> read(PacketBuffer& buffer) override;
    Result<void> write(PacketBuffer& buffer) const override;
    usize estimated_size() const override { return 1; }

    bool on_ground = false;
};

} // namespace mcserver
