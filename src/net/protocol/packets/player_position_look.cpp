#include "player_position_look.hpp"

namespace mcserver {

PacketPlayerPositionLook::PacketPlayerPositionLook(
    f64 x, f64 y, f64 stance, f64 z, f32 yaw, f32 pitch, bool on_ground)
    : x(x), y(y), stance(stance), z(z), yaw(yaw), pitch(pitch), on_ground(on_ground) {}

Result<void> PacketPlayerPositionLook::read(PacketBuffer& buffer) {
    auto x_result = buffer.read_f64();
    if (!x_result) return x_result.error();
    x = x_result.value();

    auto y_result = buffer.read_f64();
    if (!y_result) return y_result.error();
    y = y_result.value();

    auto stance_result = buffer.read_f64();
    if (!stance_result) return stance_result.error();
    stance = stance_result.value();

    auto z_result = buffer.read_f64();
    if (!z_result) return z_result.error();
    z = z_result.value();

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

Result<void> PacketPlayerPositionLook::write(PacketBuffer& buffer) const {
    buffer.write_f64(x);
    buffer.write_f64(y);
    buffer.write_f64(stance);
    buffer.write_f64(z);
    buffer.write_f32(yaw);
    buffer.write_f32(pitch);
    buffer.write_bool(on_ground);
    return Result<void>();
}

} // namespace mcserver
