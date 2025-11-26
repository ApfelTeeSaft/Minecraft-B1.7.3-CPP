#include "handshake.hpp"

namespace mcserver {

PacketHandshake::PacketHandshake(std::string username)
    : username(std::move(username)) {}

Result<void> PacketHandshake::read(PacketBuffer& buffer) {
    auto username_result = buffer.read_string(32);
    if (!username_result) {
        return username_result.error();
    }

    username = username_result.value();
    return Result<void>();
}

Result<void> PacketHandshake::write(PacketBuffer& buffer) const {
    buffer.write_string(username);
    return Result<void>();
}

usize PacketHandshake::estimated_size() const {
    return 4 + username.length() * 2 + 4;
}

} // namespace mcserver
