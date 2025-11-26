#pragma once

#include "net/protocol/packet.hpp"
#include "util/types.hpp"

namespace mcserver {

// Packet 16: BlockItemSwitch (Client→Server)
// Sent when player switches their currently held item (hotbar slot 0-8)
class PacketBlockItemSwitch : public Packet {
public:
    PacketBlockItemSwitch() = default;
    explicit PacketBlockItemSwitch(i16 slot);

    PacketId get_id() const override { return PacketId::BlockItemSwitch; }
    Result<void> read(PacketBuffer& buffer) override;
    Result<void> write(PacketBuffer& buffer) const override;
    usize estimated_size() const override { return 2; }

    i16 slot = 0;  // Hotbar slot (0-8)
};

} // namespace mcserver
