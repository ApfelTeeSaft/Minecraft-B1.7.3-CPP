#pragma once

#include "net/protocol/packet.hpp"
#include "util/types.hpp"

namespace mcserver {

// Entity action states
enum class EntityActionState : i8 {
    Crouch = 1,       // Start sneaking
    Uncrouch = 2,     // Stop sneaking
    LeaveBed = 3,     // Wake up from bed
    StartSprinting = 4,
    StopSprinting = 5
};

// Packet 19 - Entity Action
// Sent when a player performs an action (sneak, wake up, etc.)
// Server-bound only
class PacketEntityAction : public Packet {
public:
    i32 entity_id;
    EntityActionState state;

    PacketEntityAction() : entity_id(0), state(EntityActionState::Crouch) {}

    PacketEntityAction(i32 eid, EntityActionState s)
        : entity_id(eid), state(s) {}

    PacketId get_id() const override {
        return PacketId::EntityAction;
    }

    usize estimated_size() const override {
        return 5; // 1 byte ID + 4 bytes entity_id + 1 byte state
    }

    Result<void> read(PacketBuffer& buffer) override {
        auto eid_result = buffer.read_i32();
        if (!eid_result) return eid_result.error();
        entity_id = eid_result.value();

        auto state_result = buffer.read_i8();
        if (!state_result) return state_result.error();
        state = static_cast<EntityActionState>(state_result.value());

        return {};
    }

    Result<void> write(PacketBuffer& buffer) const override {
        buffer.write_i32(entity_id);
        buffer.write_i8(static_cast<i8>(state));
        return {};
    }
};

} // namespace mcserver
