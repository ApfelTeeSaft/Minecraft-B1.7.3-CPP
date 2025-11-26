#include "update_time.hpp"

namespace mcserver {

PacketUpdateTime::PacketUpdateTime(i64 time) : time(time) {}

Result<void> PacketUpdateTime::read(PacketBuffer& buffer) {
    auto time_result = buffer.read_i64();
    if (!time_result) return time_result.error();
    time = time_result.value();
    return Result<void>();
}

Result<void> PacketUpdateTime::write(PacketBuffer& buffer) const {
    buffer.write_i64(time);
    return Result<void>();
}

} // namespace mcserver
