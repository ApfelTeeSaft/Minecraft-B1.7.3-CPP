#pragma once

#include "net/protocol/packet.hpp"

namespace mcserver {

// Packet 0: KeepAlive
class PacketKeepAlive : public Packet {
public:
    PacketKeepAlive() = default;

    PacketId get_id() const override { return PacketId::KeepAlive; }
    Result<void> read(PacketBuffer& buffer) override;
    Result<void> write(PacketBuffer& buffer) const override;
    usize estimated_size() const override { return 0; }
};

} // namespace mcserver
