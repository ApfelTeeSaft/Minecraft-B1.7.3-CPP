#pragma once

#include "net/protocol/packet.hpp"

namespace mcserver {

// Packet 3: Chat
// Bidirectional - server can send messages, client can send messages
class PacketChat : public Packet {
public:
    PacketChat() = default;
    explicit PacketChat(std::string message);

    PacketId get_id() const override { return PacketId::Chat; }
    Result<void> read(PacketBuffer& buffer) override;
    Result<void> write(PacketBuffer& buffer) const override;
    usize estimated_size() const override;

    std::string message;
};

} // namespace mcserver
