#include "entity_look.hpp"

namespace mcserver {

PacketEntityLook::PacketEntityLook(i32 entity_id, i8 yaw, i8 pitch)
    : entity_id(entity_id)
    , yaw(yaw)
    , pitch(pitch) {}

Result<void> PacketEntityLook::read(PacketBuffer& buffer) {
    auto entity_id_result = buffer.read_i32();
    if (!entity_id_result) {
        return Result<void>(entity_id_result.error());
    }
    entity_id = entity_id_result.value();

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

Result<void> PacketEntityLook::write(PacketBuffer& buffer) const {
    buffer.write_i32(entity_id);
    buffer.write_i8(yaw);
    buffer.write_i8(pitch);

    return Result<void>();
}

} // namespace mcserver
