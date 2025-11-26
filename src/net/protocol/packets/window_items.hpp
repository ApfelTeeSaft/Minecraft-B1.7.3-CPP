#pragma once

#include "net/protocol/packet.hpp"
#include "entity/inventory/item_stack.hpp"
#include "util/types.hpp"
#include <vector>
#include <memory>

namespace mcserver {

// Packet 104: WindowItems (Server→Client)
// Sends all slots in a window/inventory at once
class PacketWindowItems : public Packet {
public:
    PacketWindowItems() = default;
    PacketWindowItems(i8 window_id, const std::vector<const ItemStack*>& items);

    PacketId get_id() const override { return PacketId::WindowItems; }
    Result<void> read(PacketBuffer& buffer) override;
    Result<void> write(PacketBuffer& buffer) const override;
    usize estimated_size() const override {
        return 3 + items.size() * 5;  // 1 byte window + 2 bytes count + 5 bytes per item
    }

    i8 window_id = 0;  // 0 = player inventory
    std::vector<std::unique_ptr<ItemStack>> items;
};

} // namespace mcserver
