#include "update_health.hpp"

namespace mcserver {

PacketUpdateHealth::PacketUpdateHealth(i16 health) : health(health) {}

Result<void> PacketUpdateHealth::read(PacketBuffer& buffer) {
    auto health_result = buffer.read_i16();
    if (!health_result) return health_result.error();
    health = health_result.value();
    return Result<void>();
}

Result<void> PacketUpdateHealth::write(PacketBuffer& buffer) const {
    buffer.write_i16(health);
    return Result<void>();
}

} // namespace mcserver
