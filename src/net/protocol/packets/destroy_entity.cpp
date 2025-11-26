#include "destroy_entity.hpp"

namespace mcserver {

PacketDestroyEntity::PacketDestroyEntity(i32 entity_id)
    : entity_id(entity_id) {}

Result<void> PacketDestroyEntity::read(PacketBuffer& buffer) {
    auto entity_id_result = buffer.read_i32();
    if (!entity_id_result) {
        return Result<void>(entity_id_result.error());
    }
    entity_id = entity_id_result.value();

    return Result<void>();
}

Result<void> PacketDestroyEntity::write(PacketBuffer& buffer) const {
    buffer.write_i32(entity_id);
    return Result<void>();
}

} // namespace mcserver
