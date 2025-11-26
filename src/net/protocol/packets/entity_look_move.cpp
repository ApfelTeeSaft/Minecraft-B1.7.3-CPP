#include "entity_look_move.hpp"

namespace mcserver {

PacketEntityLookMove::PacketEntityLookMove(i32 entity_id, i8 dx, i8 dy, i8 dz, i8 yaw, i8 pitch)
    : entity_id(entity_id)
    , dx(dx)
    , dy(dy)
    , dz(dz)
    , yaw(yaw)
    , pitch(pitch) {}

Result<void> PacketEntityLookMove::read(PacketBuffer& buffer) {
    auto entity_id_result = buffer.read_i32();
    if (!entity_id_result) {
        return Result<void>(entity_id_result.error());
    }
    entity_id = entity_id_result.value();

    auto dx_result = buffer.read_i8();
    if (!dx_result) {
        return Result<void>(dx_result.error());
    }
    dx = dx_result.value();

    auto dy_result = buffer.read_i8();
    if (!dy_result) {
        return Result<void>(dy_result.error());
    }
    dy = dy_result.value();

    auto dz_result = buffer.read_i8();
    if (!dz_result) {
        return Result<void>(dz_result.error());
    }
    dz = dz_result.value();

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

    return Result<void>();
}

Result<void> PacketEntityLookMove::write(PacketBuffer& buffer) const {
    buffer.write_i32(entity_id);
    buffer.write_i8(dx);
    buffer.write_i8(dy);
    buffer.write_i8(dz);
    buffer.write_i8(yaw);
    buffer.write_i8(pitch);

    return Result<void>();
}

} // namespace mcserver
