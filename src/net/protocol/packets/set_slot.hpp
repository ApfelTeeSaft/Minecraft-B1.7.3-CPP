#pragma once

#include "net/protocol/packet.hpp"
#include "entity/inventory/item_stack.hpp"
#include "util/types.hpp"
#include <memory>

namespace mcserver {

// Packet 103: SetSlot (Server→Client, Client→Server)
// Updates a single inventory slot
class PacketSetSlot : public Packet {
public:
    PacketSetSlot() = default;
    PacketSetSlot(i8 window_id, i16 slot, const ItemStack* item_stack);

    PacketId get_id() const override { return PacketId::SetSlot; }
    Result<void> read(PacketBuffer& buffer) override;
    Result<void> write(PacketBuffer& buffer) const override;
    usize estimated_size() const override { return 8; }

    i8 window_id = 0;      // 0 = player inventory
    i16 slot = 0;          // Slot index
    std::unique_ptr<ItemStack> item_stack;  // nullptr = empty slot
};

} // namespace mcserver
