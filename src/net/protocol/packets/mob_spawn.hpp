#pragma once

#include "net/protocol/packet.hpp"
#include "entity/mob/mob_type.hpp"
#include "entity/mob/mob_metadata.hpp"
#include "util/types.hpp"

namespace mcserver {

class Mob;

// Packet 24: MobSpawn (Server→Client)
// Spawns a mob entity in the world
class PacketMobSpawn : public Packet {
public:
    PacketMobSpawn() = default;
    explicit PacketMobSpawn(const Mob* mob);

    PacketId get_id() const override { return PacketId::MobSpawn; }
    Result<void> read(PacketBuffer& buffer) override;
    Result<void> write(PacketBuffer& buffer) const override;
    usize estimated_size() const override { return 20; }

    i32 entity_id = 0;
    MobType mob_type = MobType::Pig;
    i32 x_position = 0;  // Position * 32
    i32 y_position = 0;  // Position * 32
    i32 z_position = 0;  // Position * 32
    i8 yaw = 0;          // Rotation * 256 / 360
    i8 pitch = 0;        // Rotation * 256 / 360
    MobMetadata metadata;  // Mob metadata (DataWatcher)
};

} // namespace mcserver
