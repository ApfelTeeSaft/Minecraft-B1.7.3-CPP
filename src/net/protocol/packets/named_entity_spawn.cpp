#include "named_entity_spawn.hpp"

namespace mcserver {

PacketNamedEntitySpawn::PacketNamedEntitySpawn(i32 entity_id, const std::string& player_name,
                                               i32 x, i32 y, i32 z, i8 yaw, i8 pitch, i16 current_item)
    : entity_id(entity_id)
    , player_name(player_name)
    , x(x)
    , y(y)
    , z(z)
    , yaw(yaw)
    , pitch(pitch)
    , current_item(current_item) {}

Result<void> PacketNamedEntitySpawn::read(PacketBuffer& buffer) {
    auto entity_id_result = buffer.read_i32();
    if (!entity_id_result) {
        return Result<void>(entity_id_result.error());
    }
    entity_id = entity_id_result.value();

    auto name_result = buffer.read_string();
    if (!name_result) {
        return Result<void>(name_result.error());
    }
    player_name = name_result.value();

    auto x_result = buffer.read_i32();
    if (!x_result) {
        return Result<void>(x_result.error());
    }
    x = x_result.value();

    auto y_result = buffer.read_i32();
    if (!y_result) {
        return Result<void>(y_result.error());
    }
    y = y_result.value();

    auto z_result = buffer.read_i32();
    if (!z_result) {
        return Result<void>(z_result.error());
    }
    z = z_result.value();

    auto yaw_result = buffer.read_i8();
    if (!yaw_result) {
        return Result<void>(yaw_result.error());
    }
    yaw = yaw_result.value();

    auto pitch_result = buffer.read_i8();
    if (!pitch_result) {
        return Result<void>(pitch_result.error());
    }
    pitch = pitch_result.value();

    auto item_result = buffer.read_i16();
    if (!item_result) {
        return Result<void>(item_result.error());
    }
    current_item = item_result.value();

    return Result<void>();
}

Result<void> PacketNamedEntitySpawn::write(PacketBuffer& buffer) const {
    buffer.write_i32(entity_id);
    buffer.write_string(player_name);
    buffer.write_i32(x);
    buffer.write_i32(y);
    buffer.write_i32(z);
    buffer.write_i8(yaw);
    buffer.write_i8(pitch);
    buffer.write_i16(current_item);

    return Result<void>();
}

} // namespace mcserver
