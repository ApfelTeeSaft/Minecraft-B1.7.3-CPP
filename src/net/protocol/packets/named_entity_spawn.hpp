#pragma once

#include "net/protocol/packet.hpp"
#include <string>

namespace mcserver {

// Packet 20: NamedEntitySpawn
// Server -> Client
// Spawns another player entity in the world
class PacketNamedEntitySpawn : public Packet {
public:
    PacketNamedEntitySpawn() = default;
    PacketNamedEntitySpawn(i32 entity_id, const std::string& player_name,
                           i32 x, i32 y, i32 z, i8 yaw, i8 pitch, i16 current_item);

    PacketId get_id() const override { return PacketId::NamedEntitySpawn; }
    Result<void> read(PacketBuffer& buffer) override;
    Result<void> write(PacketBuffer& buffer) const override;
    usize estimated_size() const override {
        return 4 + 2 + player_name.length() * 2 + 4 + 4 + 4 + 1 + 1 + 2;
    }

    i32 entity_id = 0;
    std::string player_name;
    i32 x = 0;           // Absolute position in fixed-point (actual * 32)
    i32 y = 0;
    i32 z = 0;
    i8 yaw = 0;          // Rotation: angle * 256 / 360
    i8 pitch = 0;
    i16 current_item = 0; // Held item ID
};

} // namespace mcserver
