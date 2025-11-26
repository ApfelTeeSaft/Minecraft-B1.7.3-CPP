#include "net/protocol/packet.hpp"
#include "net/protocol/packets/handshake.hpp"
#include "net/protocol/packets/login.hpp"
#include "net/protocol/packets/keepalive.hpp"
#include <iostream>
#include <cassert>

using namespace mcserver;

int test_packet() {
    std::cout << "Testing packet serialization...\n";

    // Test handshake packet
    {
        PacketHandshake packet("TestUser");
        PacketBuffer buffer;

        auto write_result = packet.write(buffer);
        assert(write_result.is_ok());

        buffer.reset_position();

        PacketHandshake read_packet;
        auto read_result = read_packet.read(buffer);
        assert(read_result.is_ok());
        assert(read_packet.username == "TestUser");

        std::cout << "  ✓ Handshake packet\n";
    }

    // Test login packet
    {
        PacketLogin packet("Player123", 14, 123456789L, 0);
        PacketBuffer buffer;

        auto write_result = packet.write(buffer);
        assert(write_result.is_ok());

        buffer.reset_position();

        PacketLogin read_packet;
        auto read_result = read_packet.read(buffer);
        assert(read_result.is_ok());
        assert(read_packet.username == "Player123");
        assert(read_packet.protocol_version == 14);
        assert(read_packet.map_seed == 123456789L);
        assert(read_packet.dimension == 0);

        std::cout << "  ✓ Login packet\n";
    }

    // Test packet buffer primitives
    {
        PacketBuffer buffer;

        buffer.write_i32(42);
        buffer.write_i64(1234567890123L);
        buffer.write_string("Hello");
        buffer.write_bool(true);

        buffer.reset_position();

        auto i32_result = buffer.read_i32();
        assert(i32_result.is_ok() && i32_result.value() == 42);

        auto i64_result = buffer.read_i64();
        assert(i64_result.is_ok() && i64_result.value() == 1234567890123L);

        auto str_result = buffer.read_string();
        assert(str_result.is_ok() && str_result.value() == "Hello");

        auto bool_result = buffer.read_bool();
        assert(bool_result.is_ok() && bool_result.value() == true);

        std::cout << "  ✓ PacketBuffer primitives\n";
    }

    return 0;
}
