#pragma once

#include "net/protocol/packet.hpp"

namespace mcserver {

// Packet 50: PreChunk
// Tells client to load or unload a chunk
// Must be sent before MapChunk for that chunk
class PacketPreChunk : public Packet {
public:
    PacketPreChunk() = default;
    PacketPreChunk(i32 chunk_x, i32 chunk_z, bool load);

    PacketId get_id() const override { return PacketId::PreChunk; }
    Result<void> read(PacketBuffer& buffer) override;
    Result<void> write(PacketBuffer& buffer) const override;
    usize estimated_size() const override { return 9; } // 4 + 4 + 1

    i32 chunk_x = 0;
    i32 chunk_z = 0;
    bool load = true; // true = load, false = unload
};

} // namespace mcserver
