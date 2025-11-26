#pragma once

#include "net/protocol/packet.hpp"

namespace mcserver {

// Packet 255: Kick/Disconnect
// Server -> Client
// Sent to disconnect a client with a reason
class PacketKick : public Packet {
public:
    PacketKick() = default;
    explicit PacketKick(std::string reason);

    PacketId get_id() const override { return PacketId::Kick; }
    Result<void> read(PacketBuffer& buffer) override;
    Result<void> write(PacketBuffer& buffer) const override;
    usize estimated_size() const override;

    std::string reason;
};

} // namespace mcserver
