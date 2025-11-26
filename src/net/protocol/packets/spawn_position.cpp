#include "spawn_position.hpp"

namespace mcserver {

PacketSpawnPosition::PacketSpawnPosition(i32 x, i32 y, i32 z)
    : x(x), y(y), z(z) {}

Result<void> PacketSpawnPosition::read(PacketBuffer& buffer) {
    auto x_result = buffer.read_i32();
    if (!x_result) return x_result.error();
    x = x_result.value();

    auto y_result = buffer.read_i32();
    if (!y_result) return y_result.error();
    y = y_result.value();

    auto z_result = buffer.read_i32();
    if (!z_result) return z_result.error();
    z = z_result.value();

    return Result<void>();
}

Result<void> PacketSpawnPosition::write(PacketBuffer& buffer) const {
    buffer.write_i32(x);
    buffer.write_i32(y);
    buffer.write_i32(z);
    return Result<void>();
}

} // namespace mcserver
