#include "block_dig.hpp"

namespace mcserver {

PacketBlockDig::PacketBlockDig(DigStatus status, i32 x, i8 y, i32 z, i8 face)
    : status(status)
    , x(x)
    , y(y)
    , z(z)
    , face(face) {}

Result<void> PacketBlockDig::read(PacketBuffer& buffer) {
    auto status_result = buffer.read_u8();
    if (!status_result) {
        return Result<void>(status_result.error());
    }
    status = static_cast<DigStatus>(status_result.value());

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

    auto face_result = buffer.read_i8();
    if (!face_result) {
        return Result<void>(face_result.error());
    }
    face = face_result.value();

    return Result<void>();
}

Result<void> PacketBlockDig::write(PacketBuffer& buffer) const {
    buffer.write_u8(static_cast<u8>(status));
    buffer.write_i32(x);
    buffer.write_i8(y);
    buffer.write_i32(z);
    buffer.write_i8(face);

    return Result<void>();
}

} // namespace mcserver
