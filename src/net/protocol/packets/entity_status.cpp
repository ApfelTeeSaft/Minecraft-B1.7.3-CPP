#include "net/protocol/packets/entity_status.hpp"

namespace mcserver {

PacketEntityStatus::PacketEntityStatus(i32 entity_id, i8 status)
    : entity_id(entity_id), status(status) {
}

Result<void> PacketEntityStatus::read(PacketBuffer& buffer) {
    auto entity_id_result = buffer.read_i32();
    if (!entity_id_result) return entity_id_result.error();
    entity_id = entity_id_result.value();

    auto status_result = buffer.read_i8();
    if (!status_result) return status_result.error();
    status = status_result.value();

    return Result<void>();
}

Result<void> PacketEntityStatus::write(PacketBuffer& buffer) const {
    buffer.write_i32(entity_id);
    buffer.write_i8(status);
    return Result<void>();
}

} // namespace mcserver
