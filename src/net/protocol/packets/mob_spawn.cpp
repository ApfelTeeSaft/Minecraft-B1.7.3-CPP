#include "net/protocol/packets/mob_spawn.hpp"
#include "entity/mob/mob.hpp"
#include <cmath>

namespace mcserver {

PacketMobSpawn::PacketMobSpawn(const Mob* mob) {
    if (!mob) {
        return;
    }

    entity_id = mob->get_entity_id();
    mob_type = mob->get_mob_type();

    // Convert position from double to fixed-point (multiply by 32)
    x_position = static_cast<i32>(std::floor(mob->get_x() * 32.0));
    y_position = static_cast<i32>(std::floor(mob->get_y() * 32.0));
    z_position = static_cast<i32>(std::floor(mob->get_z() * 32.0));

    // Convert rotation from degrees to byte (0-255)
    yaw = static_cast<i8>(static_cast<i32>(mob->get_yaw() * 256.0f / 360.0f) & 0xFF);
    pitch = static_cast<i8>(static_cast<i32>(mob->get_pitch() * 256.0f / 360.0f) & 0xFF);

    // Copy metadata
    if (mob->get_metadata()) {
        metadata = *mob->get_metadata();
    }
}

Result<void> PacketMobSpawn::read(PacketBuffer& buffer) {
    auto entity_id_result = buffer.read_i32();
    if (!entity_id_result) return entity_id_result.error();
    entity_id = entity_id_result.value();

    auto type_result = buffer.read_i8();
    if (!type_result) return type_result.error();
    mob_type = static_cast<MobType>(type_result.value());

    auto x_result = buffer.read_i32();
    if (!x_result) return x_result.error();
    x_position = x_result.value();

    auto y_result = buffer.read_i32();
    if (!y_result) return y_result.error();
    y_position = y_result.value();

    auto z_result = buffer.read_i32();
    if (!z_result) return z_result.error();
    z_position = z_result.value();

    auto yaw_result = buffer.read_i8();
    if (!yaw_result) return yaw_result.error();
    yaw = yaw_result.value();

    auto pitch_result = buffer.read_i8();
    if (!pitch_result) return pitch_result.error();
    pitch = pitch_result.value();

    // Note: Skipping metadata reading for initial implementation
    // In full implementation, would read DataWatcher metadata here

    return Result<void>();
}

Result<void> PacketMobSpawn::write(PacketBuffer& buffer) const {
    buffer.write_i32(entity_id);
    buffer.write_i8(static_cast<i8>(mob_type));
    buffer.write_i32(x_position);
    buffer.write_i32(y_position);
    buffer.write_i32(z_position);
    buffer.write_i8(yaw);
    buffer.write_i8(pitch);

    // Write DataWatcher metadata (Beta 1.7.3 format)
    // Format: (type_id << 5 | index) followed by value
    // Terminated with 0x7F
    for (const auto& [index, entry] : metadata.get_all()) {
        // Write the key byte: (type << 5) | index
        u8 key = (static_cast<u8>(entry.type) << 5) | (entry.index & 0x1F);
        buffer.write_u8(key);

        // Write the value based on type
        switch (entry.type) {
            case MetadataType::Byte:
                buffer.write_i8(std::get<i8>(entry.value));
                break;
            case MetadataType::Short:
                buffer.write_i16(std::get<i16>(entry.value));
                break;
            case MetadataType::Int:
                buffer.write_i32(std::get<i32>(entry.value));
                break;
            case MetadataType::Float:
                buffer.write_f32(std::get<f32>(entry.value));
                break;
            case MetadataType::String:
                buffer.write_string(std::get<std::string>(entry.value));
                break;
            default:
                break;
        }
    }

    // Write terminator (0x7F) to indicate end of metadata
    buffer.write_i8(0x7F);

    return Result<void>();
}

} // namespace mcserver
