#include "keepalive.hpp"

namespace mcserver {

Result<void> PacketKeepAlive::read(PacketBuffer& buffer) {
    // KeepAlive packet has no data
    (void)buffer;
    return Result<void>();
}

Result<void> PacketKeepAlive::write(PacketBuffer& buffer) const {
    // KeepAlive packet has no data
    (void)buffer;
    return Result<void>();
}

} // namespace mcserver
