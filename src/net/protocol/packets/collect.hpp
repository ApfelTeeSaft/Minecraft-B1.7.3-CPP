#pragma once

#include "net/protocol/packet.hpp"
#include "util/types.hpp"

namespace mcserver {

// Packet 22 - Collect Item
// Sent when a player picks up an item
// Server->Client only
class PacketCollect : public Packet {
public:
    i32 collected_entity_id;  // The item entity that was picked up
    i32 collector_entity_id;  // The player who picked it up

    PacketCollect()
        : collected_entity_id(0)
        , collector_entity_id(0) {}

    PacketCollect(i32 item_eid, i32 player_eid)
        : collected_entity_id(item_eid)
        , collector_entity_id(player_eid) {}

    PacketId get_id() const override {
        return PacketId::Collect;
    }

    usize estimated_size() const override {
        return 8; // 4+4 = 8 bytes
    }

    Result<void> read(PacketBuffer& buffer) override {
        auto collected_result = buffer.read_i32();
        if (!collected_result) return collected_result.error();
        collected_entity_id = collected_result.value();

        auto collector_result = buffer.read_i32();
        if (!collector_result) return collector_result.error();
        collector_entity_id = collector_result.value();

        return {};
    }

    Result<void> write(PacketBuffer& buffer) const override {
        buffer.write_i32(collected_entity_id);
        buffer.write_i32(collector_entity_id);
        return {};
    }
};

} // namespace mcserver
