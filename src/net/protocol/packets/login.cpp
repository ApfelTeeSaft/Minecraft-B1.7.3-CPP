#include "login.hpp"

namespace mcserver {

PacketLogin::PacketLogin(std::string username, i32 protocol_version, i64 map_seed, i8 dimension)
    : protocol_version(protocol_version)
    , username(std::move(username))
    , map_seed(map_seed)
    , dimension(dimension) {}

Result<void> PacketLogin::read(PacketBuffer& buffer) {
    auto protocol_result = buffer.read_i32();
    if (!protocol_result) return protocol_result.error();
    protocol_version = protocol_result.value();

    auto username_result = buffer.read_string(16);
    if (!username_result) return username_result.error();
    username = username_result.value();

    auto seed_result = buffer.read_i64();
    if (!seed_result) return seed_result.error();
    map_seed = seed_result.value();

    auto dimension_result = buffer.read_i8();
    if (!dimension_result) return dimension_result.error();
    dimension = dimension_result.value();

    return Result<void>();
}

Result<void> PacketLogin::write(PacketBuffer& buffer) const {
    buffer.write_i32(protocol_version);
    buffer.write_string(username);
    buffer.write_i64(map_seed);
    buffer.write_i8(dimension);
    return Result<void>();
}

usize PacketLogin::estimated_size() const {
    return 4 + 2 + username.length() * 2 + 8 + 1;
}

} // namespace mcserver
