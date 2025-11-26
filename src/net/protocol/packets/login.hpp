#pragma once

#include "net/protocol/packet.hpp"

namespace mcserver {

// Packet 1: Login
class PacketLogin : public Packet {
public:
    PacketLogin() = default;
    PacketLogin(std::string username, i32 protocol_version, i64 map_seed, i8 dimension);

    PacketId get_id() const override { return PacketId::Login; }
    Result<void> read(PacketBuffer& buffer) override;
    Result<void> write(PacketBuffer& buffer) const override;
    usize estimated_size() const override;

    i32 protocol_version = 0;
    std::string username;
    i64 map_seed = 0;
    i8 dimension = 0;
};

} // namespace mcserver
