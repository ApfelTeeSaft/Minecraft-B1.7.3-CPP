add_library(net STATIC
    transport/network_manager.cpp
    transport/network_manager.hpp
    transport/chunk_streaming_manager.cpp
    transport/chunk_streaming_manager.hpp
    session/client_session.cpp
    session/client_session.hpp
    protocol/packet.cpp
    protocol/packet.hpp
    protocol/packet_handler.cpp
    protocol/packet_handler.hpp
    protocol/packets/handshake.cpp
    protocol/packets/handshake.hpp
    protocol/packets/login.cpp
    protocol/packets/login.hpp
    protocol/packets/keepalive.cpp
    protocol/packets/keepalive.hpp
    protocol/packets/spawn_position.cpp
    protocol/packets/spawn_position.hpp
    protocol/packets/update_time.cpp
    protocol/packets/update_time.hpp
    protocol/packets/player_position_look.cpp
    protocol/packets/player_position_look.hpp
    protocol/packets/pre_chunk.cpp
    protocol/packets/pre_chunk.hpp
    protocol/packets/map_chunk.cpp
    protocol/packets/map_chunk.hpp
    protocol/packets/chat.cpp
    protocol/packets/chat.hpp
    protocol/packets/update_health.cpp
    protocol/packets/update_health.hpp
    protocol/packets/respawn.cpp
    protocol/packets/respawn.hpp
    protocol/packets/kick.cpp
    protocol/packets/kick.hpp
    protocol/packets/player_flying.cpp
    protocol/packets/player_flying.hpp
    protocol/packets/player_position.cpp
    protocol/packets/player_position.hpp
    protocol/packets/player_look.cpp
    protocol/packets/player_look.hpp
    protocol/packets/named_entity_spawn.cpp
    protocol/packets/named_entity_spawn.hpp
    protocol/packets/destroy_entity.cpp
    protocol/packets/destroy_entity.hpp
    protocol/packets/entity_relative_move.cpp
    protocol/packets/entity_relative_move.hpp
    protocol/packets/entity_look.cpp
    protocol/packets/entity_look.hpp
    protocol/packets/entity_look_move.cpp
    protocol/packets/entity_look_move.hpp
    protocol/packets/block_dig.cpp
    protocol/packets/block_dig.hpp
    protocol/packets/place.cpp
    protocol/packets/place.hpp
    protocol/packets/block_change.cpp
    protocol/packets/block_change.hpp
    protocol/packets/block_item_switch.cpp
    protocol/packets/block_item_switch.hpp
    protocol/packets/set_slot.cpp
    protocol/packets/set_slot.hpp
    protocol/packets/window_items.cpp
    protocol/packets/window_items.hpp
    protocol/packets/mob_spawn.cpp
    protocol/packets/mob_spawn.hpp
    protocol/packets/entity_status.cpp
    protocol/packets/entity_status.hpp
)

target_include_directories(net PUBLIC
    ${CMAKE_SOURCE_DIR}/src
)

target_link_libraries(net PUBLIC core util platform ZLIB::ZLIB)

set_target_properties(net PROPERTIES FOLDER "Network")
