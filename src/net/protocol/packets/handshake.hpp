#pragma once

#include "net/protocol/packet.hpp"

namespace mcserver {

// Packet 2: Handshake
class PacketHandshake : public Packet {
public:
    PacketHandshake() = default;
    explicit PacketHandshake(std::string username);

    PacketId get_id() const override { return PacketId::Handshake; }
    Result<void> read(PacketBuffer& buffer) override;
    Result<void> write(PacketBuffer& buffer) const override;
    usize estimated_size() const override;

    std::string username;
};

} // namespace mcserver
