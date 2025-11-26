#pragma once

#include "net/protocol/packet.hpp"
#include "util/types.hpp"

namespace mcserver {

// Animation types
enum class AnimationType : i8 {
    NoAnimation = 0,
    SwingArm = 1,
    Damage = 2,
    LeaveBed = 3,
    EatFood = 5,
    Unknown = 102,
    Crouch = 104,
    UnCrouch = 105
};

// Packet 18 - Animation
// Sent when a player animates (e.g., arm swing)
// Bidirectional - can be sent by client or server
class PacketAnimation : public Packet {
public:
    i32 entity_id;
    AnimationType animation;

    PacketAnimation() : entity_id(0), animation(AnimationType::NoAnimation) {}

    PacketAnimation(i32 eid, AnimationType anim)
        : entity_id(eid), animation(anim) {}

    PacketId get_id() const override {
        return PacketId::Animation;
    }

    usize estimated_size() const override {
        return 5; // 1 byte ID + 4 bytes entity_id + 1 byte animation
    }

    Result<void> read(PacketBuffer& buffer) override {
        auto eid_result = buffer.read_i32();
        if (!eid_result) return eid_result.error();
        entity_id = eid_result.value();

        auto anim_result = buffer.read_i8();
        if (!anim_result) return anim_result.error();
        animation = static_cast<AnimationType>(anim_result.value());

        return {};
    }

    Result<void> write(PacketBuffer& buffer) const override {
        buffer.write_i32(entity_id);
        buffer.write_i8(static_cast<i8>(animation));
        return {};
    }
};

} // namespace mcserver
