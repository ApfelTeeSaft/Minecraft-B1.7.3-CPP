#pragma once

#include "net/protocol/packet.hpp"

namespace mcserver {

// Packet 29: DestroyEntity
// Server -> Client
// Sent when an entity is destroyed/removed
class PacketDestroyEntity : public Packet {
public:
    PacketDestroyEntity() = default;
    explicit PacketDestroyEntity(i32 entity_id);

    PacketId get_id() const override { return PacketId::DestroyEntity; }
    Result<void> read(PacketBuffer& buffer) override;
    Result<void> write(PacketBuffer& buffer) const override;
    usize estimated_size() const override { return 4; } // Entity ID only

    i32 entity_id = 0;
};

} // namespace mcserver
