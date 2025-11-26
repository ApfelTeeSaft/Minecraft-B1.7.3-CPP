#include "net/protocol/packets/block_item_switch.hpp"

namespace mcserver {

PacketBlockItemSwitch::PacketBlockItemSwitch(i16 slot) : slot(slot) {}

Result<void> PacketBlockItemSwitch::read(PacketBuffer& buffer) {
    auto slot_result = buffer.read_i16();
    if (!slot_result) return slot_result.error();
    slot = slot_result.value();
    return Result<void>();
}

Result<void> PacketBlockItemSwitch::write(PacketBuffer& buffer) const {
    buffer.write_i16(slot);
    return Result<void>();
}

} // namespace mcserver
