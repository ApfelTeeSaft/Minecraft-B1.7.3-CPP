#include "player_flying.hpp"

namespace mcserver {

PacketPlayerFlying::PacketPlayerFlying(bool on_ground) : on_ground(on_ground) {}

Result<void> PacketPlayerFlying::read(PacketBuffer& buffer) {
    auto ground_result = buffer.read_bool();
    if (!ground_result) return ground_result.error();
    on_ground = ground_result.value();
    return Result<void>();
}

Result<void> PacketPlayerFlying::write(PacketBuffer& buffer) const {
    buffer.write_bool(on_ground);
    return Result<void>();
}

} // namespace mcserver
