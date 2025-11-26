#pragma once

#include "net/protocol/packet.hpp"
#include "util/types.hpp"

namespace mcserver {

// Packet 21 - Pickup Spawn (Item Entity)
// Sent when an item is dropped in the world
// Server->Client only
class PacketPickupSpawn : public Packet {
public:
    i32 entity_id;
    i16 item_id;
    i8 count;
    i16 damage;
    i32 x;  // Fixed-point (x * 32)
    i32 y;  // Fixed-point (y * 32)
    i32 z;  // Fixed-point (z * 32)
    i8 rotation;  // Yaw
    i8 pitch;
    i8 roll;

    PacketPickupSpawn()
        : entity_id(0)
        , item_id(0)
        , count(1)
        , damage(0)
        , x(0)
        , y(0)
        , z(0)
        , rotation(0)
        , pitch(0)
        , roll(0) {}

    PacketPickupSpawn(i32 eid, i16 item, i8 cnt, i16 dmg,
                      f64 pos_x, f64 pos_y, f64 pos_z,
                      i8 rot, i8 p, i8 r)
        : entity_id(eid)
        , item_id(item)
        , count(cnt)
        , damage(dmg)
        , x(static_cast<i32>(pos_x * 32.0))
        , y(static_cast<i32>(pos_y * 32.0))
        , z(static_cast<i32>(pos_z * 32.0))
        , rotation(rot)
        , pitch(p)
        , roll(r) {}

    PacketId get_id() const override {
        return PacketId::PickupSpawn;
    }

    usize estimated_size() const override {
        return 24; // 4+2+1+2+4+4+4+1+1+1 = 24 bytes
    }

    Result<void> read(PacketBuffer& buffer) override {
        auto eid_result = buffer.read_i32();
        if (!eid_result) return eid_result.error();
        entity_id = eid_result.value();

        auto item_result = buffer.read_i16();
        if (!item_result) return item_result.error();
        item_id = item_result.value();

        auto count_result = buffer.read_i8();
        if (!count_result) return count_result.error();
        count = count_result.value();

        auto damage_result = buffer.read_i16();
        if (!damage_result) return damage_result.error();
        damage = damage_result.value();

        auto x_result = buffer.read_i32();
        if (!x_result) return x_result.error();
        x = x_result.value();

        auto y_result = buffer.read_i32();
        if (!y_result) return y_result.error();
        y = y_result.value();

        auto z_result = buffer.read_i32();
        if (!z_result) return z_result.error();
        z = z_result.value();

        auto rot_result = buffer.read_i8();
        if (!rot_result) return rot_result.error();
        rotation = rot_result.value();

        auto pitch_result = buffer.read_i8();
        if (!pitch_result) return pitch_result.error();
        pitch = pitch_result.value();

        auto roll_result = buffer.read_i8();
        if (!roll_result) return roll_result.error();
        roll = roll_result.value();

        return {};
    }

    Result<void> write(PacketBuffer& buffer) const override {
        buffer.write_i32(entity_id);
        buffer.write_i16(item_id);
        buffer.write_i8(count);
        buffer.write_i16(damage);
        buffer.write_i32(x);
        buffer.write_i32(y);
        buffer.write_i32(z);
        buffer.write_i8(rotation);
        buffer.write_i8(pitch);
        buffer.write_i8(roll);
        return {};
    }
};

} // namespace mcserver
