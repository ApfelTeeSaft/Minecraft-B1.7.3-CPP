#pragma once

#include "net/protocol/packet.hpp"

namespace mcserver {

// Packet 9: Respawn
// Server -> Client
// Sent to respawn player or change dimension
class PacketRespawn : public Packet {
public:
    PacketRespawn() = default;
    PacketRespawn(i8 dimension, i8 difficulty, i8 creative_mode, i16 world_height, i64 map_seed);

    PacketId get_id() const override { return PacketId::Respawn; }
    Result<void> read(PacketBuffer& buffer) override;
    Result<void> write(PacketBuffer& buffer) const override;
    usize estimated_size() const override { return 13; } // 1+1+1+2+8

    i8 dimension = 0;        // 0 = overworld, -1 = nether
    i8 difficulty = 1;       // 0=peaceful, 1=easy, 2=normal, 3=hard
    i8 creative_mode = 0;    // 0=survival, 1=creative
    i16 world_height = 128;  // Always 128 in Beta 1.7.3
    i64 map_seed = 0;
};

} // namespace mcserver
