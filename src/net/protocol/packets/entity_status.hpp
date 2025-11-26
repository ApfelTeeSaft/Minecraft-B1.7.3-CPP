#pragma once

#include "net/protocol/packet.hpp"
#include "util/types.hpp"

namespace mcserver {

// Packet 38: EntityStatus (Server→Client)
// Sent when an entity's status changes (damage, death, etc.)
class PacketEntityStatus : public Packet {
public:
    PacketEntityStatus() = default;
    PacketEntityStatus(i32 entity_id, i8 status);

    PacketId get_id() const override { return PacketId::EntityStatus; }
    Result<void> read(PacketBuffer& buffer) override;
    Result<void> write(PacketBuffer& buffer) const override;
    usize estimated_size() const override { return 5; }  // 4 + 1

    i32 entity_id = 0;
    i8 status = 0;  // 2 = hurt, 3 = dead, etc.
};

} // namespace mcserver
