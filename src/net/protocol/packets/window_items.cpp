#include "net/protocol/packets/window_items.hpp"

namespace mcserver {

PacketWindowItems::PacketWindowItems(i8 window_id, const std::vector<const ItemStack*>& items)
    : window_id(window_id) {
    this->items.reserve(items.size());
    for (const auto* item : items) {
        if (item && !item->is_empty()) {
            this->items.push_back(item->clone());
        } else {
            this->items.push_back(std::make_unique<ItemStack>());
        }
    }
}

Result<void> PacketWindowItems::read(PacketBuffer& buffer) {
    auto window_result = buffer.read_i8();
    if (!window_result) return window_result.error();
    window_id = window_result.value();

    auto count_result = buffer.read_i16();
    if (!count_result) return count_result.error();
    i16 count = count_result.value();

    if (count < 0 || count > 256) {
        return ErrorCode::InvalidArgument;
    }

    items.clear();
    items.reserve(count);

    for (i16 i = 0; i < count; ++i) {
        auto item_id_result = buffer.read_i16();
        if (!item_id_result) return item_id_result.error();
        i16 item_id = item_id_result.value();

        if (item_id < 0) {
            // Empty slot
            items.push_back(std::make_unique<ItemStack>());
        } else {
            // Read count and damage
            auto item_count_result = buffer.read_i8();
            if (!item_count_result) return item_count_result.error();
            i8 item_count = item_count_result.value();

            auto damage_result = buffer.read_i16();
            if (!damage_result) return damage_result.error();
            i16 damage = damage_result.value();

            items.push_back(std::make_unique<ItemStack>(item_id, item_count, damage));
        }
    }

    return Result<void>();
}

Result<void> PacketWindowItems::write(PacketBuffer& buffer) const {
    buffer.write_i8(window_id);
    buffer.write_i16(static_cast<i16>(items.size()));

    for (const auto& item : items) {
        if (!item || item->is_empty()) {
            // Empty slot
            buffer.write_i16(-1);
        } else {
            // Write item data
            buffer.write_i16(item->get_item_id());
            buffer.write_i8(item->get_count());
            buffer.write_i16(item->get_damage());
        }
    }

    return Result<void>();
}

} // namespace mcserver
