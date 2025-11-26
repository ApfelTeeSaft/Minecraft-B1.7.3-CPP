#include "player_look.hpp"

namespace mcserver {

PacketPlayerLook::PacketPlayerLook(f32 yaw, f32 pitch, bool on_ground)
    : yaw(yaw), pitch(pitch), on_ground(on_ground) {}

Result<void> PacketPlayerLook::read(PacketBuffer& buffer) {
    auto yaw_result = buffer.read_f32();
    if (!yaw_result) return yaw_result.error();
    yaw = yaw_result.value();

    auto pitch_result = buffer.read_f32();
    if (!pitch_result) return pitch_result.error();
    pitch = pitch_result.value();

    auto ground_result = buffer.read_bool();
    if (!ground_result) return ground_result.error();
    on_ground = ground_result.value();

    return Result<void>();
}

Result<void> PacketPlayerLook::write(PacketBuffer& buffer) const {
    buffer.write_f32(yaw);
    buffer.write_f32(pitch);
    buffer.write_bool(on_ground);
    return Result<void>();
}

} // namespace mcserver
