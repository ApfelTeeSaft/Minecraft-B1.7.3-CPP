#include "respawn.hpp"

namespace mcserver {

PacketRespawn::PacketRespawn(i8 dimension, i8 difficulty, i8 creative_mode, i16 world_height, i64 map_seed)
    : dimension(dimension), difficulty(difficulty), creative_mode(creative_mode),
      world_height(world_height), map_seed(map_seed) {}

Result<void> PacketRespawn::read(PacketBuffer& buffer) {
    auto dimension_result = buffer.read_i8();
    if (!dimension_result) return dimension_result.error();
    dimension = dimension_result.value();

    auto difficulty_result = buffer.read_i8();
    if (!difficulty_result) return difficulty_result.error();
    difficulty = difficulty_result.value();

    auto creative_result = buffer.read_i8();
    if (!creative_result) return creative_result.error();
    creative_mode = creative_result.value();

    auto height_result = buffer.read_i16();
    if (!height_result) return height_result.error();
    world_height = height_result.value();

    auto seed_result = buffer.read_i64();
    if (!seed_result) return seed_result.error();
    map_seed = seed_result.value();

    return Result<void>();
}

Result<void> PacketRespawn::write(PacketBuffer& buffer) const {
    buffer.write_i8(dimension);
    buffer.write_i8(difficulty);
    buffer.write_i8(creative_mode);
    buffer.write_i16(world_height);
    buffer.write_i64(map_seed);
    return Result<void>();
}

} // namespace mcserver
