#include "place.hpp"

namespace mcserver {

PacketPlace::PacketPlace(i32 x, i8 y, i32 z, i8 direction, i16 block_item_id, u8 amount, i16 damage)
    : x(x)
    , y(y)
    , z(z)
    , direction(direction)
    , block_item_id(block_item_id)
    , amount(amount)
    , damage(damage) {}

Result<void> PacketPlace::read(PacketBuffer& buffer) {
    auto x_result = buffer.read_i32();
    if (!x_result) {
        return Result<void>(x_result.error());
    }
    x = x_result.value();

    auto y_result = buffer.read_i8();
    if (!y_result) {
        return Result<void>(y_result.error());
    }
    y = y_result.value();

    auto z_result = buffer.read_i32();
    if (!z_result) {
        return Result<void>(z_result.error());
    }
    z = z_result.value();

    auto dir_result = buffer.read_i8();
    if (!dir_result) {
        return Result<void>(dir_result.error());
    }
    direction = dir_result.value();

    auto item_result = buffer.read_i16();
    if (!item_result) {
        return Result<void>(item_result.error());
    }
    block_item_id = item_result.value();

    // Only read amount and damage if item is present
    if (block_item_id != -1) {
        auto amount_result = buffer.read_u8();
        if (!amount_result) {
            return Result<void>(amount_result.error());
        }
        amount = amount_result.value();

        auto damage_result = buffer.read_i16();
        if (!damage_result) {
            return Result<void>(damage_result.error());
        }
        damage = damage_result.value();
    }

    return Result<void>();
}

Result<void> PacketPlace::write(PacketBuffer& buffer) const {
    buffer.write_i32(x);
    buffer.write_i8(y);
    buffer.write_i32(z);
    buffer.write_i8(direction);
    buffer.write_i16(block_item_id);

    // Only write amount and damage if item is present
    if (block_item_id != -1) {
        buffer.write_u8(amount);
        buffer.write_i16(damage);
    }

    return Result<void>();
}

} // namespace mcserver
