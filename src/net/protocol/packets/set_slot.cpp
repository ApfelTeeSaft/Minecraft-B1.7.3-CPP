#include "net/protocol/packets/set_slot.hpp"

namespace mcserver {

PacketSetSlot::PacketSetSlot(i8 window_id, i16 slot, const ItemStack* item_stack)
    : window_id(window_id), slot(slot) {
    if (item_stack && !item_stack->is_empty()) {
        this->item_stack = item_stack->clone();
    }
}

Result<void> PacketSetSlot::read(PacketBuffer& buffer) {
    auto window_result = buffer.read_i8();
    if (!window_result) return window_result.error();
    window_id = window_result.value();

    auto slot_result = buffer.read_i16();
    if (!slot_result) return slot_result.error();
    slot = slot_result.value();

    // Read item stack
    auto item_id_result = buffer.read_i16();
    if (!item_id_result) return item_id_result.error();
    i16 item_id = item_id_result.value();

    if (item_id < 0) {
        // Empty slot
        item_stack = nullptr;
    } else {
        // Read count and damage
        auto count_result = buffer.read_i8();
        if (!count_result) return count_result.error();
        i8 count = count_result.value();

        auto damage_result = buffer.read_i16();
        if (!damage_result) return damage_result.error();
        i16 damage = damage_result.value();

        item_stack = std::make_unique<ItemStack>(item_id, count, damage);
    }

    return Result<void>();
}

Result<void> PacketSetSlot::write(PacketBuffer& buffer) const {
    buffer.write_i8(window_id);
    buffer.write_i16(slot);

    if (!item_stack || item_stack->is_empty()) {
        // Empty slot
        buffer.write_i16(-1);
    } else {
        // Write item data
        buffer.write_i16(item_stack->get_item_id());
        buffer.write_i8(item_stack->get_count());
        buffer.write_i16(item_stack->get_damage());
    }

    return Result<void>();
}

} // namespace mcserver
