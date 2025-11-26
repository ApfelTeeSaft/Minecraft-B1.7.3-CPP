#pragma once

#include "net/protocol/packet.hpp"
#include "entity/inventory/item_stack.hpp"
#include "util/types.hpp"
#include <memory>

namespace mcserver {

// Packet 102 - Window Click
// Sent when a player clicks in an inventory window
// Server-bound only
class PacketWindowClick : public Packet {
public:
    i8 window_id;           // 0 for player inventory
    i16 slot;               // Slot that was clicked
    i8 right_click;         // 0 = left click, 1 = right click
    i16 action_number;      // Incremental counter for server-side validation
    bool shift;             // Was shift held during click
    std::unique_ptr<ItemStack> clicked_item;  // Item in cursor (may be null)

    PacketWindowClick()
        : window_id(0)
        , slot(0)
        , right_click(0)
        , action_number(0)
        , shift(false)
        , clicked_item(nullptr) {}

    PacketWindowClick(i8 wid, i16 s, i8 rc, i16 action, bool shift_held, const ItemStack* item)
        : window_id(wid)
        , slot(s)
        , right_click(rc)
        , action_number(action)
        , shift(shift_held)
        , clicked_item(item ? std::make_unique<ItemStack>(*item) : nullptr) {}

    PacketId get_id() const override {
        return PacketId::WindowClick;
    }

    usize estimated_size() const override {
        // Variable size: 1 byte ID + 1 byte window_id + 2 bytes slot + 1 byte right_click
        // + 2 bytes action + 1 byte shift + item data (2-7 bytes)
        return 16; // Approximate size
    }

    Result<void> read(PacketBuffer& buffer) override {
        auto wid_result = buffer.read_i8();
        if (!wid_result) return wid_result.error();
        window_id = wid_result.value();

        auto slot_result = buffer.read_i16();
        if (!slot_result) return slot_result.error();
        slot = slot_result.value();

        auto rc_result = buffer.read_i8();
        if (!rc_result) return rc_result.error();
        right_click = rc_result.value();

        auto action_result = buffer.read_i16();
        if (!action_result) return action_result.error();
        action_number = action_result.value();

        auto shift_result = buffer.read_bool();
        if (!shift_result) return shift_result.error();
        shift = shift_result.value();

        // Read item stack (may be -1 for empty)
        auto item_id_result = buffer.read_i16();
        if (!item_id_result) return item_id_result.error();
        i16 item_id = item_id_result.value();

        if (item_id != -1) {
            auto count_result = buffer.read_i8();
            if (!count_result) return count_result.error();

            auto damage_result = buffer.read_i16();
            if (!damage_result) return damage_result.error();

            clicked_item = std::make_unique<ItemStack>(
                static_cast<i16>(item_id),
                static_cast<i8>(count_result.value()),
                static_cast<i16>(damage_result.value())
            );
        } else {
            clicked_item = nullptr;
        }

        return {};
    }

    Result<void> write(PacketBuffer& buffer) const override {
        buffer.write_i8(window_id);
        buffer.write_i16(slot);
        buffer.write_i8(right_click);
        buffer.write_i16(action_number);
        buffer.write_bool(shift);

        if (clicked_item) {
            buffer.write_i16(clicked_item->get_item_id());
            buffer.write_i8(clicked_item->get_count());
            buffer.write_i16(clicked_item->get_damage());
        } else {
            buffer.write_i16(-1);
        }

        return {};
    }
};

} // namespace mcserver
