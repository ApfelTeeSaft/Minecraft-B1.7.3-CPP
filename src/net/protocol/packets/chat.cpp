#include "chat.hpp"

namespace mcserver {

PacketChat::PacketChat(std::string message) : message(std::move(message)) {}

Result<void> PacketChat::read(PacketBuffer& buffer) {
    auto msg_result = buffer.read_string(119); // Max length in Beta 1.7.3
    if (!msg_result) return msg_result.error();
    message = msg_result.value();
    return Result<void>();
}

Result<void> PacketChat::write(PacketBuffer& buffer) const {
    buffer.write_string(message);
    return Result<void>();
}

usize PacketChat::estimated_size() const {
    // 2 bytes for string length + (string length * 2 for UTF-16)
    return 2 + (message.length() * 2);
}

} // namespace mcserver
