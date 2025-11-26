#include "client_session.hpp"
#include "net/transport/chunk_streaming_manager.hpp"
#include "net/protocol/packets/handshake.hpp"
#include "net/protocol/packets/login.hpp"
#include "net/protocol/packets/keepalive.hpp"
#include "net/protocol/packets/spawn_position.hpp"
#include "net/protocol/packets/player_position_look.hpp"
#include "net/protocol/packets/pre_chunk.hpp"
#include "net/protocol/packets/map_chunk.hpp"
#include "net/protocol/packets/chat.hpp"
#include "net/protocol/packets/update_health.hpp"
#include "net/protocol/packets/player_position.hpp"
#include "net/protocol/packets/player_look.hpp"
#include "net/protocol/packets/player_flying.hpp"
#include "net/protocol/packets/block_dig.hpp"
#include "net/protocol/packets/place.hpp"
#include "net/protocol/packets/block_item_switch.hpp"
#include "net/protocol/packets/set_slot.hpp"
#include "net/protocol/packets/window_items.hpp"
#include "net/protocol/packets/animation.hpp"
#include "net/protocol/packets/entity_action.hpp"
#include "net/protocol/packets/window_click.hpp"
#include "net/protocol/packets/close_window.hpp"
#include "net/protocol/packets/kick.hpp"
#include "net/protocol/packets/use_entity.hpp"
#include "net/protocol/packets/entity_status.hpp"
#include "world/chunk/chunk_manager.hpp"
#include "entity/entity_manager.hpp"
#include "entity/mob/mob_manager.hpp"
#include "entity/item/item_entity_manager.hpp"
#include "world/block/block_manager.hpp"
#include "storage/player/player_data_manager.hpp"
#include "admin/admin_manager.hpp"
#include "util/log/logger.hpp"
#include <cstring>

namespace mcserver {

// Helper function to convert internal slot index to protocol slot index
static i32 internal_to_protocol_slot(i32 internal_slot) {
    // Internal layout:
    // 0-8: Hotbar → Protocol 36-44
    // 9-35: Main → Protocol 9-35
    // 36-39: Armor → Protocol 5-8
    // 40-43: Crafting grid → Protocol 1-4
    // 44: Crafting output → Protocol 0

    if (internal_slot >= 0 && internal_slot <= 8) {
        return 36 + internal_slot;  // Hotbar
    } else if (internal_slot >= 9 && internal_slot <= 35) {
        return internal_slot;  // Main inventory (same)
    } else if (internal_slot >= 36 && internal_slot <= 39) {
        return 5 + (internal_slot - 36);  // Armor
    } else if (internal_slot >= 40 && internal_slot <= 43) {
        return 1 + (internal_slot - 40);  // Crafting grid
    } else if (internal_slot == 44) {
        return 0;  // Crafting output
    }
    return -1;  // Invalid
}

// Helper function to convert protocol slot index to internal slot index
static i32 protocol_to_internal_slot(i32 protocol_slot) {
    // Protocol layout:
    // 0: Crafting output → Internal 44
    // 1-4: Crafting grid → Internal 40-43
    // 5-8: Armor → Internal 36-39
    // 9-35: Main → Internal 9-35
    // 36-44: Hotbar → Internal 0-8

    if (protocol_slot == 0) {
        return 44;  // Crafting output
    } else if (protocol_slot >= 1 && protocol_slot <= 4) {
        return 40 + (protocol_slot - 1);  // Crafting grid
    } else if (protocol_slot >= 5 && protocol_slot <= 8) {
        return 36 + (protocol_slot - 5);  // Armor
    } else if (protocol_slot >= 9 && protocol_slot <= 35) {
        return protocol_slot;  // Main inventory (same)
    } else if (protocol_slot >= 36 && protocol_slot <= 44) {
        return protocol_slot - 36;  // Hotbar
    }
    return -1;  // Invalid
}

ClientSession::ClientSession(Socket socket, ChunkManager* chunk_manager,
                            EntityManager* entity_manager,
                            BlockManager* block_manager,
                            MobManager* mob_manager,
                            ItemEntityManager* item_entity_manager,
                            ChunkStreamingManager* chunk_streaming_manager,
                            PlayerDataManager* player_data_manager,
                            AdminManager* admin_manager,
                            ChatBroadcastCallback chat_callback,
                            PlayerJoinCallback join_callback,
                            PlayerLeaveCallback leave_callback)
    : socket_(std::move(socket))
    , chunk_manager_(chunk_manager)
    , entity_manager_(entity_manager)
    , block_manager_(block_manager)
    , mob_manager_(mob_manager)
    , item_entity_manager_(item_entity_manager)
    , chunk_streaming_manager_(chunk_streaming_manager)
    , player_data_manager_(player_data_manager)
    , admin_manager_(admin_manager)
    , chat_callback_(std::move(chat_callback))
    , join_callback_(std::move(join_callback))
    , leave_callback_(std::move(leave_callback))
    , state_(SessionState::Handshake) {
    recv_buffer_.reserve(8192);
    send_buffer_.reserve(8192);

    // Set socket to non-blocking
    socket_.set_non_blocking(true);
    socket_.set_tcp_nodelay(true);
}

ClientSession::~ClientSession() {
    // Save player data before disconnecting
    if (player_ && player_data_manager_) {
        auto save_result = player_data_manager_->save_player(*player_);
        if (!save_result) {
            LOG_ERROR_CAT("Failed to save player data for " + username_,
                         LogCategory::Storage);
        }
    }
    disconnect();
}

void ClientSession::process() {
    if (!is_connected()) {
        return;
    }

    // Receive data
    byte temp_buffer[4096];
    auto recv_result = socket_.receive(temp_buffer, sizeof(temp_buffer));

    if (recv_result) {
        isize received = recv_result.value();
        if (received > 0) {
            recv_buffer_.insert(recv_buffer_.end(), temp_buffer, temp_buffer + received);
        } else if (received == 0) {
            // Connection closed
            disconnect("Connection closed by client");
            return;
        }
    } else if (recv_result.error() != ErrorCode::Timeout) {
        // Real error
        disconnect("Network error");
        return;
    }

    // Process packets
    while (recv_buffer_.size() > 0) {
        // Need at least packet ID
        if (recv_buffer_.size() < 1) {
            break;
        }

        u8 packet_id = static_cast<u8>(recv_buffer_[0]);

        // Create packet buffer (skip the ID byte for now)
        PacketBuffer buffer(std::vector<byte>(recv_buffer_.begin() + 1, recv_buffer_.end()));

        bool packet_processed = false;

        // Handle based on state
        if (state_ == SessionState::Handshake) {
            if (packet_id == static_cast<u8>(PacketId::Handshake)) {
                handle_handshake(buffer);
                packet_processed = true;
            } else {
                disconnect("Invalid packet in handshake state");
                return;
            }
        } else if (state_ == SessionState::Login) {
            if (packet_id == static_cast<u8>(PacketId::Login)) {
                handle_login(buffer);
                packet_processed = true;
            } else {
                disconnect("Invalid packet in login state");
                return;
            }
        } else if (state_ == SessionState::Play) {
            packet_processed = handle_play_packet(packet_id, buffer);
        }

        // Only remove bytes if packet was successfully processed
        if (packet_processed) {
            // Remove processed bytes (ID + consumed data)
            usize bytes_consumed = 1 + buffer.position();
            if (bytes_consumed <= recv_buffer_.size()) {
                recv_buffer_.erase(recv_buffer_.begin(), recv_buffer_.begin() + bytes_consumed);
            } else {
                // Packet read more than available - this should never happen
                // but if it does, it indicates a bug. Disconnect to prevent corruption.
                disconnect("Packet buffer overflow");
                return;
            }
        } else {
            // Packet wasn't processed (either partial or unknown)
            // Check if this looks like an unknown packet ID (desync)
            if (recv_buffer_.size() >= 10) {
                // We have enough data, but packet wasn't recognized - desync!
                disconnect("Invalid packet ID: " + std::to_string(packet_id) + " (possible desynchronization)");
                return;
            } else {
                // Not enough data yet, wait for more
                break;
            }
        }
    }
}

void ClientSession::send_packet(const Packet& packet) {
    if (!is_connected()) {
        return;
    }

    PacketBuffer buffer;

    // Write packet ID
    buffer.write_u8(static_cast<u8>(packet.get_id()));

    // Write packet data
    auto write_result = packet.write(buffer);
    if (!write_result) {
        LOG_ERROR_CAT("Failed to serialize packet", LogCategory::Network);
        return;
    }

    // Send data
    const auto& data = buffer.data();
    auto send_result = socket_.send(data.data(), data.size());

    if (!send_result) {
        LOG_ERROR_CAT("Failed to send packet", LogCategory::Network);
        disconnect("Send error");
    }
}

void ClientSession::disconnect(const std::string& reason) {
    if (state_ == SessionState::Disconnected) {
        return;
    }

    if (!reason.empty()) {
        LOG_INFO_CAT(std::string("Client disconnected: ") + reason, LogCategory::Network);
    }

    // Mark as disconnected FIRST to prevent recursive disconnect calls
    // This stops send_packet from attempting to send during cleanup
    state_ = SessionState::Disconnected;
    socket_.close();

    // Remove player from chunk streaming (this may try to send PreChunk packets, but send_packet will now bail early)
    if (chunk_streaming_manager_) {
        chunk_streaming_manager_->remove_player(this);
    }

    // Despawn player from all other clients and remove from entity manager
    if (player_ && entity_manager_) {
        entity_manager_->despawn_entity_for_all(player_->get_entity_id());
        entity_manager_->remove_player(player_->get_entity_id());
    }

    // Notify that player left if they were in play state
    if (player_ && leave_callback_) {
        leave_callback_(username_);
    }
}

void ClientSession::handle_handshake(PacketBuffer& buffer) {
    PacketHandshake packet;
    auto read_result = packet.read(buffer);

    if (!read_result) {
        disconnect("Failed to parse handshake");
        return;
    }

    username_ = packet.username;

    LOG_INFO_CAT(std::string("Handshake from: ") + username_, LogCategory::Network);

    // Send handshake response (connectionHash "-" for offline mode)
    PacketHandshake response("-");
    send_packet(response);

    state_ = SessionState::Login;
}

void ClientSession::handle_login(PacketBuffer& buffer) {
    PacketLogin packet;
    auto read_result = packet.read(buffer);

    if (!read_result) {
        disconnect("Failed to parse login");
        return;
    }

    LOG_INFO_CAT(std::string("Login from: ") + packet.username, LogCategory::Network);

    // Check for duplicate username
    if (entity_manager_) {
        auto existing_players = entity_manager_->get_all_players();
        for (const auto* existing_player : existing_players) {
            if (existing_player->get_username() == username_) {
                LOG_WARNING_CAT("Duplicate username detected: " + username_ + " - rejecting login", LogCategory::Network);
                PacketKick kick_packet("A player with that name is already connected");
                send_packet(kick_packet);
                disconnect("Duplicate username");
                return;
            }
        }
    }

    // Send login response
    PacketLogin response(packet.username, 14, 0, 0);
    send_packet(response);

    state_ = SessionState::Play;

    LOG_INFO_CAT(std::string("Client logged in: ") + username_, LogCategory::Network);

    // Create player entity with unique ID from entity manager
    i32 entity_id = 1;
    if (entity_manager_) {
        entity_id = entity_manager_->get_id_manager()->allocate();
    }
    player_ = std::make_unique<Player>(username_, entity_id);

    // Try to load existing player data
    bool loaded_data = false;
    if (player_data_manager_) {
        auto load_result = player_data_manager_->load_player(*player_);
        if (!load_result) {
            LOG_ERROR_CAT("Failed to load player data for " + username_,
                         LogCategory::Storage);
        } else {
            loaded_data = load_result.value();
        }
    }

    // If no saved data, set default spawn position
    if (!loaded_data) {
        LOG_INFO_CAT("Setting default spawn position for " + username_, LogCategory::Entity);
        // Find spawn surface height at (0, 0)
        f64 spawn_y = 64.0; // Default fallback
        if (chunk_manager_) {
            LOG_INFO_CAT("Getting spawn chunk (0, 0) for " + username_, LogCategory::Entity);
            Chunk* spawn_chunk = chunk_manager_->get_chunk(0, 0);
            LOG_INFO_CAT("Got spawn chunk, searching for surface for " + username_, LogCategory::Entity);
            if (spawn_chunk) {
                // Implement findTopSolidBlock algorithm from original Minecraft
                // Start from top (127) and search downward for first non-air block
                bool found_solid = false;
                for (i32 y = 127; y > 0; y--) {
                    u8 block = spawn_chunk->get_block(0, y, 0);
                    if (block != 0) {  // Non-air block found
                        // Return y + 1 (spawn 1 block above the solid block)
                        spawn_y = static_cast<f64>(y) + 1.0;
                        found_solid = true;
                        break;
                    }
                }
                if (!found_solid) {
                    spawn_y = 64.0;  // Fallback to sea level
                }
            }
        }
        player_->set_position(0.5, spawn_y, 0.5);
        LOG_INFO_CAT("New player " + username_ + " spawned at default position (0.5, " +
                    std::to_string(spawn_y) + ", 0.5)", LogCategory::Entity);
    }

    // Register player with entity manager
    if (entity_manager_) {
        entity_manager_->add_player(player_.get(), this);
    }

    // Notify that player joined
    if (join_callback_) {
        join_callback_(username_);
    }

    // Send spawn position and initial chunks
    send_initial_chunks();

    // Send initial health (20 = full health)
    PacketUpdateHealth health_packet(20);
    send_packet(health_packet);

    // Send initial inventory to client
    send_full_inventory();

    // Spawn existing entities for this player
    if (entity_manager_) {
        entity_manager_->spawn_existing_entities_for(this);
    }

    // Spawn existing mobs for this player
    if (mob_manager_) {
        mob_manager_->spawn_existing_mobs_for(this);
    }

    // Spawn this player to other nearby players
    if (entity_manager_) {
        entity_manager_->spawn_entity_for_nearby_players(player_.get(), this);
    }
}

bool ClientSession::handle_play_packet(u8 packet_id, PacketBuffer& buffer) {
    PacketId pid = static_cast<PacketId>(packet_id);

    switch (pid) {
        case PacketId::KeepAlive: {
            // Echo keep-alive back
            PacketKeepAlive response;
            send_packet(response);
            return true;
        }

        case PacketId::Chat: {
            PacketChat chat;
            auto read_result = chat.read(buffer);
            if (!read_result) {
                return false; // Insufficient data, wait for more
            }

            // Check if this is a command (starts with '/')
            if (!chat.message.empty() && chat.message[0] == '/') {
                handle_command(chat.message);
                return true;
            }

            if (chat_callback_) {
                // Broadcast chat message to all players
                chat_callback_(chat.message, username_);
                LOG_INFO_CAT(username_ + ": " + chat.message, LogCategory::General);
            }
            return true;
        }

        case PacketId::Flying: {
            PacketPlayerFlying packet;
            auto read_result = packet.read(buffer);
            if (!read_result) {
                return false; // Insufficient data
            }
            if (player_) {
                player_->set_on_ground(packet.on_ground);
            }
            return true;
        }

        case PacketId::PlayerPosition: {
            PacketPlayerPosition packet;
            auto read_result = packet.read(buffer);
            if (!read_result) {
                return false; // Insufficient data
            }
            if (player_) {
                player_->set_position(packet.x, packet.y, packet.z);
                player_->set_on_ground(packet.on_ground);
            }
            return true;
        }

        case PacketId::PlayerLook: {
            PacketPlayerLook packet;
            auto read_result = packet.read(buffer);
            if (!read_result) {
                return false; // Insufficient data
            }
            if (player_) {
                player_->set_rotation(packet.yaw, packet.pitch);
                player_->set_on_ground(packet.on_ground);
            }
            return true;
        }

        case PacketId::PlayerLookMove: {
            // This is packet 13 - already handled as PlayerPositionLook
            PacketPlayerPositionLook packet;
            auto read_result = packet.read(buffer);
            if (!read_result) {
                return false; // Insufficient data
            }
            if (player_) {
                player_->set_position(packet.x, packet.y, packet.z);
                player_->set_rotation(packet.yaw, packet.pitch);
                player_->set_on_ground(packet.on_ground);
            }
            return true;
        }

        case PacketId::BlockDig: {
            PacketBlockDig packet;
            auto read_result = packet.read(buffer);
            if (!read_result) {
                return false; // Insufficient data
            }
            if (block_manager_) {
                // Only handle finished digging for now
                if (packet.status == DigStatus::Finished) {
                    auto result = block_manager_->break_block(packet.x, packet.y, packet.z);
                    if (!result) {
                        LOG_DEBUG_CAT("Failed to break block at (" +
                                     std::to_string(packet.x) + ", " +
                                     std::to_string(packet.y) + ", " +
                                     std::to_string(packet.z) + ")",
                                     LogCategory::World);
                    }
                }
            }
            return true;
        }

        case PacketId::Place: {
            PacketPlace packet;
            auto read_result = packet.read(buffer);
            if (!read_result) {
                return false; // Insufficient data
            }
            if (block_manager_ && player_) {
                // Only handle block placement (not items)
                if (packet.block_item_id > 0 && packet.block_item_id < 256) {
                    // Calculate placement position based on clicked face
                    i32 place_x = packet.x;
                    i8 place_y = packet.y;
                    i32 place_z = packet.z;

                    // Adjust position based on face clicked
                    switch (packet.direction) {
                        case 0: place_y--; break; // -Y
                        case 1: place_y++; break; // +Y
                        case 2: place_z--; break; // -Z
                        case 3: place_z++; break; // +Z
                        case 4: place_x--; break; // -X
                        case 5: place_x++; break; // +X
                    }

                    // Check if block would collide with player bounding box
                    // Player bounding box: 0.6 wide (x/z) x 1.8 tall (y)
                    // Block bounding box: 1x1x1 from (place_x, place_y, place_z) to (place_x+1, place_y+1, place_z+1)
                    f64 player_x = player_->get_x();
                    f64 player_y = player_->get_y();
                    f64 player_z = player_->get_z();

                    // Player bounding box (AABB)
                    f64 player_min_x = player_x - 0.3;
                    f64 player_max_x = player_x + 0.3;
                    f64 player_min_y = player_y;
                    f64 player_max_y = player_y + 1.8;
                    f64 player_min_z = player_z - 0.3;
                    f64 player_max_z = player_z + 0.3;

                    // Block bounding box
                    f64 block_min_x = static_cast<f64>(place_x);
                    f64 block_max_x = static_cast<f64>(place_x + 1);
                    f64 block_min_y = static_cast<f64>(place_y);
                    f64 block_max_y = static_cast<f64>(place_y + 1);
                    f64 block_min_z = static_cast<f64>(place_z);
                    f64 block_max_z = static_cast<f64>(place_z + 1);

                    // Check AABB collision
                    bool collides = !(player_max_x <= block_min_x || player_min_x >= block_max_x ||
                                     player_max_y <= block_min_y || player_min_y >= block_max_y ||
                                     player_max_z <= block_min_z || player_min_z >= block_max_z);

                    if (collides) {
                        LOG_DEBUG_CAT("Cannot place block at (" + std::to_string(place_x) + ", " +
                                     std::to_string(place_y) + ", " + std::to_string(place_z) +
                                     ") - would collide with player", LogCategory::World);
                        // Send block update to client to cancel the placement
                        return true;
                    }

                    auto result = block_manager_->place_block(place_x, place_y, place_z,
                                                             static_cast<u8>(packet.block_item_id), 0);
                    if (!result) {
                        LOG_DEBUG_CAT("Failed to place block at (" +
                                     std::to_string(place_x) + ", " +
                                     std::to_string(place_y) + ", " +
                                     std::to_string(place_z) + ")",
                                     LogCategory::World);
                    }
                }
            }
            return true;
        }

        case PacketId::BlockItemSwitch: {
            PacketBlockItemSwitch packet;
            auto read_result = packet.read(buffer);
            if (!read_result) {
                return false; // Insufficient data
            }
            if (player_) {
                // Switch held item in hotbar (slots 0-8)
                if (packet.slot >= 0 && packet.slot < 9) {
                    player_->get_inventory()->set_current_slot(static_cast<i32>(packet.slot));
                    LOG_DEBUG_CAT("Player " + username_ + " switched to hotbar slot " +
                                 std::to_string(packet.slot), LogCategory::Network);
                }
            }
            return true;
        }

        case PacketId::Animation: {
            PacketAnimation packet;
            auto read_result = packet.read(buffer);
            if (!read_result) {
                return false; // Insufficient data
            }
            // Broadcast animation to other players
            if (entity_manager_) {
                auto other_players = entity_manager_->get_other_players(player_->get_entity_id());
                for (auto* other_player : other_players) {
                    ClientSession* session = entity_manager_->get_player_session(other_player->get_entity_id());
                    if (session) {
                        PacketAnimation broadcast(player_->get_entity_id(), packet.animation);
                        session->send_packet(broadcast);
                    }
                }
            }
            LOG_DEBUG_CAT("Player " + username_ + " animated: " +
                         std::to_string(static_cast<i8>(packet.animation)), LogCategory::Network);
            return true;
        }

        case PacketId::UseEntity: {
            PacketUseEntity packet;
            auto read_result = packet.read(buffer);
            if (!read_result) {
                return false; // Insufficient data
            }

            // Handle entity interaction (attack or right-click)
            if (packet.left_click) {
                // Attack/damage entity
                LOG_DEBUG_CAT("Player " + username_ + " attacked entity " +
                             std::to_string(packet.target_id), LogCategory::Network);

                // Broadcast arm swing animation to other players
                if (entity_manager_) {
                    auto other_players = entity_manager_->get_other_players(player_->get_entity_id());
                    for (auto* other_player : other_players) {
                        ClientSession* session = entity_manager_->get_player_session(other_player->get_entity_id());
                        if (session) {
                            PacketAnimation anim_packet(player_->get_entity_id(), AnimationType::SwingArm);
                            session->send_packet(anim_packet);
                        }
                    }
                }

                // Calculate damage based on held item
                i16 damage = 1;  // Base unarmed damage
                i32 current_slot = player_->get_inventory()->get_current_slot();
                const ItemStack* held_item = player_->get_inventory()->get_slot(current_slot);
                if (held_item && !held_item->is_empty()) {
                    i16 item_id = held_item->get_item_id();
                    // Calculate damage based on item type
                    // Swords
                    if (item_id == 268) damage = 5;  // Wooden Sword
                    else if (item_id == 272) damage = 6;  // Stone Sword
                    else if (item_id == 267) damage = 7;  // Iron Sword
                    else if (item_id == 283) damage = 8;  // Gold Sword
                    else if (item_id == 276) damage = 9;  // Diamond Sword
                    // Axes (less damage than swords but still effective)
                    else if (item_id == 271) damage = 4;  // Wooden Axe
                    else if (item_id == 275) damage = 5;  // Stone Axe
                    else if (item_id == 258) damage = 6;  // Iron Axe
                    else if (item_id == 286) damage = 5;  // Gold Axe
                    else if (item_id == 279) damage = 7;  // Diamond Axe
                    // Pickaxes (minimal bonus)
                    else if (item_id == 270 || item_id == 274 || item_id == 257 || item_id == 285 || item_id == 278) {
                        damage = 3;  // All pickaxes give small bonus
                    }
                    // Shovels (minimal bonus)
                    else if (item_id == 269 || item_id == 273 || item_id == 256 || item_id == 284 || item_id == 277) {
                        damage = 2;  // All shovels give tiny bonus
                    }
                }
                bool target_died = false;

                // Check if target is a mob
                if (mob_manager_) {
                    Mob* target_mob = mob_manager_->get_mob(packet.target_id);
                    if (target_mob) {
                        // Apply knockback from player position
                        target_mob->apply_knockback(player_->get_x(), player_->get_z(), 0.4f);

                        // Trigger panic mode for passive mobs
                        target_mob->on_attacked_by(player_->get_x(), player_->get_z());

                        i16 new_health = target_mob->get_health() - damage;
                        target_mob->set_health(new_health);

                        LOG_DEBUG_CAT("Mob " + std::to_string(packet.target_id) +
                                     " took " + std::to_string(damage) + " damage (health: " +
                                     std::to_string(new_health) + "/" +
                                     std::to_string(target_mob->get_max_health()) + ")",
                                     LogCategory::Entity);

                        // Broadcast hurt animation to all players
                        if (entity_manager_) {
                            PacketEntityStatus hurt_packet(packet.target_id, 2);  // Status 2 = hurt
                            auto all_players = entity_manager_->get_all_players();
                            for (auto* player : all_players) {
                                ClientSession* session = entity_manager_->get_player_session(player->get_entity_id());
                                if (session) {
                                    session->send_packet(hurt_packet);
                                }
                            }
                        }

                        // Check if mob died
                        if (target_mob->is_dead()) {
                            target_died = true;
                            LOG_INFO_CAT("Mob " + target_mob->get_name() + " (ID: " +
                                        std::to_string(packet.target_id) + ") was killed by " + username_,
                                        LogCategory::Entity);

                            // Broadcast death animation
                            if (entity_manager_) {
                                PacketEntityStatus death_packet(packet.target_id, 3);  // Status 3 = dead
                                auto all_players = entity_manager_->get_all_players();
                                for (auto* player : all_players) {
                                    ClientSession* session = entity_manager_->get_player_session(player->get_entity_id());
                                    if (session) {
                                        session->send_packet(death_packet);
                                    }
                                }
                            }

                            // Spawn death drops
                            auto drops = target_mob->get_death_drops();
                            for (const auto& [item_id, count] : drops) {
                                if (count > 0 && item_entity_manager_) {
                                    // Create item stack and spawn at mob's position
                                    ItemStack drop_item(item_id, count, 0);
                                    item_entity_manager_->spawn_item(
                                        drop_item,
                                        target_mob->get_x(),
                                        target_mob->get_y(),
                                        target_mob->get_z()
                                    );
                                }
                            }

                            // Note: Mob will be removed by mob_manager after death animation completes
                            // (when should_despawn() returns true)
                        }
                    }
                }

                // Check if target is a player (PvP)
                if (!target_died && entity_manager_) {
                    Player* target_player = entity_manager_->get_player(packet.target_id);
                    if (target_player) {
                        i16 new_health = target_player->get_health() - damage;
                        target_player->set_health(new_health);

                        LOG_DEBUG_CAT("Player " + target_player->get_username() +
                                     " took " + std::to_string(damage) + " damage (health: " +
                                     std::to_string(new_health) + "/20)",
                                     LogCategory::Entity);

                        // Broadcast hurt animation to all players
                        PacketEntityStatus hurt_packet(packet.target_id, 2);  // Status 2 = hurt
                        auto all_players = entity_manager_->get_all_players();
                        for (auto* player : all_players) {
                            ClientSession* session = entity_manager_->get_player_session(player->get_entity_id());
                            if (session) {
                                session->send_packet(hurt_packet);
                            }
                        }

                        // Update health for target player
                        ClientSession* target_session = entity_manager_->get_player_session(packet.target_id);
                        if (target_session) {
                            PacketUpdateHealth health_packet(new_health);
                            target_session->send_packet(health_packet);
                        }

                        // Check if player died
                        if (target_player->is_dead()) {
                            LOG_INFO_CAT("Player " + target_player->get_username() +
                                        " was killed by " + username_,
                                        LogCategory::Entity);

                            // Broadcast death animation
                            PacketEntityStatus death_packet(packet.target_id, 3);  // Status 3 = dead
                            for (auto* player : all_players) {
                                ClientSession* session = entity_manager_->get_player_session(player->get_entity_id());
                                if (session) {
                                    session->send_packet(death_packet);
                                }
                            }

                            // Reset player health and respawn
                            if (target_session) {
                                target_player->set_health(20);
                                PacketUpdateHealth respawn_health(20);
                                target_session->send_packet(respawn_health);
                                // TODO: Implement full respawn with position reset
                            }
                        }
                    }
                }
            } else {
                // Right-click interaction (e.g., open trade window, ride entity)
                LOG_DEBUG_CAT("Player " + username_ + " interacted with entity " +
                             std::to_string(packet.target_id), LogCategory::Network);
                // TODO: Handle entity interactions
            }

            return true;
        }

        case PacketId::EntityAction: {
            PacketEntityAction packet;
            auto read_result = packet.read(buffer);
            if (!read_result) {
                return false; // Insufficient data
            }
            if (player_) {
                // Handle entity actions (sneaking, etc.)
                switch (packet.state) {
                    case EntityActionState::Crouch:
                        player_->set_sneaking(true);
                        LOG_DEBUG_CAT("Player " + username_ + " started sneaking", LogCategory::Network);
                        break;
                    case EntityActionState::Uncrouch:
                        player_->set_sneaking(false);
                        LOG_DEBUG_CAT("Player " + username_ + " stopped sneaking", LogCategory::Network);
                        break;
                    case EntityActionState::LeaveBed:
                        // TODO: Implement bed leaving
                        LOG_DEBUG_CAT("Player " + username_ + " left bed", LogCategory::Network);
                        break;
                    case EntityActionState::StartSprinting:
                        player_->set_sprinting(true);
                        LOG_DEBUG_CAT("Player " + username_ + " started sprinting", LogCategory::Network);
                        break;
                    case EntityActionState::StopSprinting:
                        player_->set_sprinting(false);
                        LOG_DEBUG_CAT("Player " + username_ + " stopped sprinting", LogCategory::Network);
                        break;
                }
            }
            return true;
        }

        case PacketId::WindowClick: {
            PacketWindowClick packet;
            auto read_result = packet.read(buffer);
            if (!read_result) {
                return false; // Insufficient data
            }
            if (player_) {
                LOG_DEBUG_CAT("Player " + username_ + " clicked window " +
                             std::to_string(packet.window_id) + " protocol slot " +
                             std::to_string(packet.slot) + " (action: " +
                             std::to_string(packet.action_number) + ")", LogCategory::Network);

                // Handle player inventory window (window_id 0)
                if (packet.window_id == 0) {
                    // Protocol slot 0 is the crafting output
                    if (packet.slot == 0) {
                        // Get the crafting result
                        const ItemStack* result = player_->get_inventory()->get_crafting_result();
                        if (result && !result->is_empty()) {
                            // Try to add the result to the player's inventory
                            auto result_copy = std::make_unique<ItemStack>(*result);
                            i8 remaining = player_->get_inventory()->add_item(std::move(result_copy));

                            if (remaining == 0) {
                                // Successfully added - consume crafting materials
                                player_->get_inventory()->take_crafting_result();

                                // Update crafting result based on new grid
                                // (need access to recipe manager - will be null for now)
                                player_->get_inventory()->update_crafting_result(nullptr);

                                LOG_DEBUG_CAT("Player " + username_ + " crafted item", LogCategory::Entity);
                            } else {
                                LOG_DEBUG_CAT("Player " + username_ + " inventory full, cannot craft",
                                            LogCategory::Entity);
                            }

                            // Send full inventory update to sync everything
                            send_full_inventory();
                        }
                    } else {
                        // Convert protocol slot to internal slot for other slots
                        i32 internal_slot = protocol_to_internal_slot(packet.slot);
                        if (internal_slot >= 0 && internal_slot < 45) {
                            // For now, just acknowledge by sending the slot back
                            // TODO: Implement full click logic (swap, split stack, etc.)
                            send_inventory_update(internal_slot);

                            // If clicking in crafting grid, update crafting result
                            if (internal_slot >= 40 && internal_slot <= 43) {
                                player_->get_inventory()->update_crafting_result(nullptr);
                                send_inventory_update(44);  // Update crafting output
                            }
                        }
                    }
                }
            }
            return true;
        }

        case PacketId::CloseWindow: {
            PacketCloseWindow packet;
            auto read_result = packet.read(buffer);
            if (!read_result) {
                return false; // Insufficient data
            }
            // Player closed a window (inventory, chest, etc.)
            LOG_DEBUG_CAT("Player " + username_ + " closed window " +
                         std::to_string(packet.window_id), LogCategory::Network);
            // No action needed - just acknowledge
            return true;
        }

        default:
            // Unknown packet - return false to indicate it wasn't recognized
            LOG_DEBUG_CAT("Unhandled packet ID: " + std::to_string(packet_id), LogCategory::Network);
            return false;
    }
}

void ClientSession::send_initial_chunks() {
    if (!chunk_streaming_manager_) {
        LOG_ERROR_CAT("ChunkStreamingManager not available for chunk sending", LogCategory::Network);
        return;
    }

    // Spawn position (center of world)
    constexpr i32 spawn_x = 0;
    constexpr i32 spawn_y = 64;
    constexpr i32 spawn_z = 0;

    // Send spawn position
    PacketSpawnPosition spawn_packet(spawn_x, spawn_y, spawn_z);
    send_packet(spawn_packet);

    LOG_INFO_CAT("Sending initial chunks to " + username_, LogCategory::Network);

    // Use ChunkStreamingManager to send chunks in spiral pattern
    chunk_streaming_manager_->add_player(this,
                                          static_cast<f64>(spawn_x),
                                          static_cast<f64>(spawn_z));

    // Send player position (spawn + slight offset above ground)
    PacketPlayerPositionLook pos_packet;
    pos_packet.x = static_cast<f64>(spawn_x) + 0.5;
    pos_packet.y = static_cast<f64>(spawn_y) + 1.62; // Eye height
    pos_packet.stance = static_cast<f64>(spawn_y) + 1.62;
    pos_packet.z = static_cast<f64>(spawn_z) + 0.5;
    pos_packet.yaw = 0.0f;
    pos_packet.pitch = 0.0f;
    pos_packet.on_ground = false;
    send_packet(pos_packet);

    LOG_INFO_CAT("Initial chunks sent to " + username_, LogCategory::Network);
}

void ClientSession::send_full_inventory() {
    if (!player_) {
        return;
    }

    // Map internal slots to protocol slots for Beta 1.7.3 player inventory
    // Protocol layout (45 slots total):
    // - Slot 0: Crafting output
    // - Slots 1-4: Crafting grid (2x2)
    // - Slots 5-8: Armor (boots, leggings, chestplate, helmet)
    // - Slots 9-35: Main inventory (3 rows x 9)
    // - Slots 36-44: Hotbar (9 slots)
    //
    // Our internal layout (45 slots total):
    // - Slots 0-8: Hotbar
    // - Slots 9-35: Main inventory
    // - Slots 36-39: Armor (boots, leggings, chestplate, helmet)
    // - Slots 40-43: Crafting grid (2x2)
    // - Slot 44: Crafting output

    std::vector<const ItemStack*> items;
    items.reserve(45);

    // Protocol slot 0: Crafting output (internal slot 44)
    items.push_back(player_->get_inventory()->get_slot(44));

    // Protocol slots 1-4: Crafting grid (internal slots 40-43)
    for (i32 i = 40; i < 44; ++i) {
        items.push_back(player_->get_inventory()->get_slot(i));
    }

    // Protocol slots 5-8: Armor (internal slots 36-39)
    for (i32 i = 36; i < 40; ++i) {
        items.push_back(player_->get_inventory()->get_slot(i));
    }

    // Protocol slots 9-35: Main inventory (internal slots 9-35)
    for (i32 i = 9; i < 36; ++i) {
        items.push_back(player_->get_inventory()->get_slot(i));
    }

    // Protocol slots 36-44: Hotbar (internal slots 0-8)
    for (i32 i = 0; i < 9; ++i) {
        items.push_back(player_->get_inventory()->get_slot(i));
    }

    // Send full inventory (window_id 0 = player inventory)
    PacketWindowItems inventory_packet(0, items);
    send_packet(inventory_packet);

    LOG_DEBUG_CAT("Sent full inventory to " + username_, LogCategory::Network);
}

void ClientSession::send_inventory_update(i32 internal_slot) {
    if (!player_) {
        return;
    }

    if (internal_slot < 0 || internal_slot >= 45) {
        LOG_ERROR_CAT("Invalid inventory slot: " + std::to_string(internal_slot), LogCategory::Network);
        return;
    }

    const auto* item = player_->get_inventory()->get_slot(internal_slot);

    // Convert internal slot to protocol slot
    i32 protocol_slot = internal_to_protocol_slot(internal_slot);
    if (protocol_slot < 0) {
        LOG_ERROR_CAT("Failed to map internal slot " + std::to_string(internal_slot) + " to protocol slot",
                     LogCategory::Network);
        return;
    }

    // Create and send SetSlot packet (window_id 0 = player inventory)
    PacketSetSlot update_packet(0, static_cast<i16>(protocol_slot), item);
    send_packet(update_packet);

    LOG_DEBUG_CAT("Sent inventory update for internal slot " + std::to_string(internal_slot) +
                 " (protocol slot " + std::to_string(protocol_slot) + ") to " + username_,
                 LogCategory::Network);
}

void ClientSession::handle_command(const std::string& command) {
    LOG_INFO_CAT(username_ + " executed command: " + command, LogCategory::General);

    if (!admin_manager_) {
        send_chat_message("§cAdmin system not available");
        return;
    }

    // Execute command through admin manager
    CommandResult result = admin_manager_->execute_command(command, player_.get());

    // If command succeeded and might have modified state, sync it
    if (result.success) {
        if (command.find("/give") == 0) {
            // Refresh inventory to show new items
            send_full_inventory();
        } else if (command.find("/tp") == 0 && player_) {
            // Send position update to teleport player client-side
            PacketPlayerPositionLook pos_packet;
            pos_packet.x = player_->get_x();
            pos_packet.y = player_->get_y();
            pos_packet.stance = player_->get_y() + 1.62; // Eye height
            pos_packet.z = player_->get_z();
            pos_packet.yaw = player_->get_yaw();
            pos_packet.pitch = player_->get_pitch();
            pos_packet.on_ground = player_->is_on_ground();
            send_packet(pos_packet);
        }
    }

    // Send result message to player (split by newlines for multi-line messages)
    if (!result.message.empty()) {
        std::string message = result.message;
        size_t pos = 0;
        size_t newline_pos;

        // Split message by newlines and send each line separately
        while ((newline_pos = message.find('\n', pos)) != std::string::npos) {
            std::string line = message.substr(pos, newline_pos - pos);
            if (!line.empty()) {
                send_chat_message(line);
            }
            pos = newline_pos + 1;
        }

        // Send remaining part (or whole message if no newlines)
        if (pos < message.length()) {
            std::string line = message.substr(pos);
            if (!line.empty()) {
                send_chat_message(line);
            }
        }
    }
}

void ClientSession::send_chat_message(const std::string& message) {
    PacketChat chat_packet;
    chat_packet.message = message;
    send_packet(chat_packet);
}

} // namespace mcserver
