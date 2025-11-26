#pragma once

#include "util/types.hpp"
#include "util/result.hpp"
#include "util/span_util.hpp"
#include <string>
#include <vector>
#include <memory>

namespace mcserver {

// Packet IDs for Beta 1.7.3
enum class PacketId : u8 {
    KeepAlive = 0,
    Login = 1,
    Handshake = 2,
    Chat = 3,
    UpdateTime = 4,
    PlayerInventory = 5,
    SpawnPosition = 6,
    UseEntity = 7,
    UpdateHealth = 8,
    Respawn = 9,
    Flying = 10,
    PlayerPosition = 11,
    PlayerLook = 12,
    PlayerLookMove = 13,
    BlockDig = 14,
    Place = 15,
    BlockItemSwitch = 16,
    Sleep = 17,
    Animation = 18,
    EntityAction = 19,
    NamedEntitySpawn = 20,
    PickupSpawn = 21,
    Collect = 22,
    VehicleSpawn = 23,
    MobSpawn = 24,
    EntityPainting = 25,
    Position = 27,
    EntityVelocity = 28,
    DestroyEntity = 29,
    Entity = 30,
    RelEntityMove = 31,
    EntityLook = 32,
    RelEntityMoveLook = 33,
    EntityTeleport = 34,
    EntityStatus = 38,
    AttachEntity = 39,
    EntityMetadata = 40,
    PreChunk = 50,
    MapChunk = 51,
    MultiBlockChange = 52,
    BlockChange = 53,
    PlayNoteBlock = 54,
    Explosion = 60,
    DoorChange = 61,
    Bed = 70,
    Weather = 71,
    OpenWindow = 100,
    CloseWindow = 101,
    WindowClick = 102,
    SetSlot = 103,
    WindowItems = 104,
    UpdateProgressbar = 105,
    Transaction = 106,
    UpdateSign = 130,
    MapData = 131,
    Statistic = 200,
    Kick = 255
};

// Packet reader/writer for Beta 1.7.3 format
class PacketBuffer {
public:
    explicit PacketBuffer(std::vector<byte> data = {});

    // Read operations
    Result<u8> read_u8();
    Result<i8> read_i8();
    Result<u16> read_u16();
    Result<i16> read_i16();
    Result<u32> read_u32();
    Result<i32> read_i32();
    Result<u64> read_u64();
    Result<i64> read_i64();
    Result<f32> read_f32();
    Result<f64> read_f64();
    Result<bool> read_bool();
    Result<std::string> read_string(usize max_length = 32767);

    // Write operations
    void write_u8(u8 value);
    void write_i8(i8 value);
    void write_u16(u16 value);
    void write_i16(i16 value);
    void write_u32(u32 value);
    void write_i32(i32 value);
    void write_u64(u64 value);
    void write_i64(i64 value);
    void write_f32(f32 value);
    void write_f64(f64 value);
    void write_bool(bool value);
    void write_string(const std::string& str);

    // Buffer management
    const std::vector<byte>& data() const { return data_; }
    std::vector<byte>&& take_data() { return std::move(data_); }
    usize size() const { return data_.size(); }
    usize position() const { return position_; }
    void reset_position() { position_ = 0; }

private:
    std::vector<byte> data_;
    usize position_ = 0;

    bool ensure_available(usize bytes);
};

// Base packet class
class Packet {
public:
    virtual ~Packet() = default;

    virtual PacketId get_id() const = 0;
    virtual Result<void> read(PacketBuffer& buffer) = 0;
    virtual Result<void> write(PacketBuffer& buffer) const = 0;
    virtual usize estimated_size() const = 0;
};

} // namespace mcserver
