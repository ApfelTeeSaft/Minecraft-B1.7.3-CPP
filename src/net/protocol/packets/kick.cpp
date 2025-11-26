#include "kick.hpp"

namespace mcserver {

PacketKick::PacketKick(std::string reason) : reason(std::move(reason)) {}

Result<void> PacketKick::read(PacketBuffer& buffer) {
    auto reason_result = buffer.read_string(256);
    if (!reason_result) return reason_result.error();
    reason = reason_result.value();
    return Result<void>();
}

Result<void> PacketKick::write(PacketBuffer& buffer) const {
    buffer.write_string(reason);
    return Result<void>();
}

usize PacketKick::estimated_size() const {
    // 2 bytes for string length + (string length * 2 for UTF-16)
    return 2 + (reason.length() * 2);
}

} // namespace mcserver
