#include "pre_chunk.hpp"

namespace mcserver {

PacketPreChunk::PacketPreChunk(i32 chunk_x, i32 chunk_z, bool load)
    : chunk_x(chunk_x), chunk_z(chunk_z), load(load) {}

Result<void> PacketPreChunk::read(PacketBuffer& buffer) {
    auto x_result = buffer.read_i32();
    if (!x_result) return x_result.error();
    chunk_x = x_result.value();

    auto z_result = buffer.read_i32();
    if (!z_result) return z_result.error();
    chunk_z = z_result.value();

    auto load_result = buffer.read_bool();
    if (!load_result) return load_result.error();
    load = load_result.value();

    return Result<void>();
}

Result<void> PacketPreChunk::write(PacketBuffer& buffer) const {
    buffer.write_i32(chunk_x);
    buffer.write_i32(chunk_z);
    buffer.write_bool(load);
    return Result<void>();
}

} // namespace mcserver
