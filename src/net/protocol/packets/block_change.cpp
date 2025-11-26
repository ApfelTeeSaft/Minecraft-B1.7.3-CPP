#include "block_change.hpp"

namespace mcserver {

PacketBlockChange::PacketBlockChange(i32 x, i8 y, i32 z, u8 block_type, u8 block_metadata)
    : x(x)
    , y(y)
    , z(z)
    , block_type(block_type)
    , block_metadata(block_metadata) {}

Result<void> PacketBlockChange::read(PacketBuffer& buffer) {
    auto x_result = buffer.read_i32();
    if (!x_result) {
        return Result<void>(x_result.error());
    }
    x = x_result.value();

    auto y_result = buffer.read_i8();
    if (!y_result) {
        return Result<void>(y_result.error());
    }
    y = y_result.value();

    auto z_result = buffer.read_i32();
    if (!z_result) {
        return Result<void>(z_result.error());
    }
    z = z_result.value();

    auto type_result = buffer.read_u8();
    if (!type_result) {
        return Result<void>(type_result.error());
    }
    block_type = type_result.value();

    auto meta_result = buffer.read_u8();
    if (!meta_result) {
        return Result<void>(meta_result.error());
    }
    block_metadata = meta_result.value();

    return Result<void>();
}

Result<void> PacketBlockChange::write(PacketBuffer& buffer) const {
    buffer.write_i32(x);
    buffer.write_i8(y);
    buffer.write_i32(z);
    buffer.write_u8(block_type);
    buffer.write_u8(block_metadata);

    return Result<void>();
}

} // namespace mcserver
