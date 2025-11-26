#include "network_manager.hpp"
#include "net/protocol/packets/chat.hpp"
#include "net/protocol/packets/named_entity_spawn.hpp"
#include "net/protocol/packets/destroy_entity.hpp"
#include "net/protocol/packets/block_change.hpp"
#include "net/protocol/packets/map_chunk.hpp"
#include "net/protocol/packets/mob_spawn.hpp"
#include "net/protocol/packets/pickup_spawn.hpp"
#include "net/protocol/packets/collect.hpp"
#include "net/protocol/packets/entity_relative_move.hpp"
#include "net/protocol/packets/entity_look.hpp"
#include "net/protocol/packets/entity_look_move.hpp"
#include "net/protocol/packets/update_health.hpp"
#include "net/protocol/packets/entity_status.hpp"
#include "net/protocol/packets/respawn.hpp"
#include "net/protocol/packets/player_position_look.hpp"
#include "world/chunk/chunk_manager.hpp"
#include "entity/player.hpp"
#include "entity/mob/mob.hpp"
#include "entity/item/item_entity.hpp"
#include "util/log/logger.hpp"
#include <algorithm>
#include <cmath>

namespace mcserver {

NetworkManager::NetworkManager(ChunkManager* chunk_manager, const std::string& world_path)
    : chunk_manager_(chunk_manager)
    , job_system_(4)  // 4 worker threads for async operations
    , async_io_(&job_system_)
    , block_manager_(chunk_manager)
    , mob_manager_(chunk_manager)
    , item_entity_manager_(entity_manager_.get_id_manager())
    , chunk_streaming_manager_(chunk_manager, 10)  // Default view distance: 10 chunks
    , player_data_manager_(world_path, &async_io_)
    , admin_manager_() {
    // Start the job system
    job_system_.start();

    // Set up admin manager with manager references
    admin_manager_.set_chunk_manager(chunk_manager_);
    admin_manager_.set_entity_manager(&entity_manager_);
    admin_manager_.set_mob_manager(&mob_manager_);
    // Set up entity manager callbacks
    entity_manager_.set_spawn_player_callback([this](ClientSession* viewer, const Player* player) {
        this->spawn_player_to_client(viewer, player);
    });

    entity_manager_.set_despawn_entity_callback([this](ClientSession* viewer, i32 entity_id) {
        this->despawn_entity_from_client(viewer, entity_id);
    });

    // Set up block manager callbacks
    block_manager_.set_block_change_callback([this](i32 x, i8 y, i32 z, u8 block_type, u8 metadata) {
        this->broadcast_block_change(x, y, z, block_type, metadata);
    });

    block_manager_.set_chunk_update_callback([this](i32 chunk_x, i32 chunk_z) {
        this->broadcast_chunk_update(chunk_x, chunk_z);
    });

    // Set up mob manager callbacks
    mob_manager_.set_spawn_callback([this](const Mob* mob) {
        this->broadcast_mob_spawn(mob);
    });

    mob_manager_.set_movement_callback([this](i32 entity_id, f64 old_x, f64 old_y, f64 old_z,
                                              f64 new_x, f64 new_y, f64 new_z, f32 yaw, f32 pitch) {
        this->broadcast_mob_movement(entity_id, old_x, old_y, old_z, new_x, new_y, new_z, yaw, pitch);
    });

    mob_manager_.set_despawn_callback([this](i32 entity_id) {
        this->broadcast_mob_despawn(entity_id);
    });

    // Set up entity manager health change callback
    entity_manager_.set_health_change_callback([this](i32 entity_id, i16 health, bool took_damage) {
        // Send UpdateHealth packet to the player
        this->send_health_update(entity_id, health);

        // Broadcast EntityStatus packet for damage/death animation
        if (took_damage) {
            i8 status = (health <= 0) ? 3 : 2;  // 3 = dead, 2 = hurt
            this->broadcast_entity_status(entity_id, status);
        }
    });

    // Set up entity manager death callback
    entity_manager_.set_death_callback([this](i32 entity_id) {
        this->handle_player_death(entity_id);
    });

    // Set up item entity manager callbacks
    item_entity_manager_.set_spawn_callback([this](const ItemEntity* item) {
        this->broadcast_item_spawn(item);
    });

    item_entity_manager_.set_despawn_callback([this](i32 entity_id) {
        this->broadcast_item_despawn(entity_id);
    });

    item_entity_manager_.set_collect_callback([this](i32 item_entity_id, i32 collector_entity_id) {
        this->broadcast_item_collect(item_entity_id, collector_entity_id);
    });

    // Connect block manager to item entity manager
    block_manager_.set_item_entity_manager(&item_entity_manager_);
}

NetworkManager::~NetworkManager() {
    stop();
}

Result<void> NetworkManager::start(const std::string& address, u16 port) {
    auto result = listener_.start(address, port);
    if (!result) {
        return result;
    }

    LOG_INFO_CAT(std::string("Network listening on ") + address + ":" + std::to_string(port),
                 LogCategory::Network);

    return Result<void>();
}

void NetworkManager::stop() {
    listener_.stop();

    // Wait for all pending async I/O operations to complete before shutting down
    job_system_.wait_all();
    job_system_.stop();

    clients_.clear();
}

void NetworkManager::tick() {
    accept_connections();
    process_clients();

    // Update player list for hostile mob targeting and natural spawning
    // We cache this to avoid rebuilding it on every mob update
    static i32 player_list_update_counter = 0;
    if (++player_list_update_counter >= 20) {  // Update once per second
        player_list_update_counter = 0;
        player_list_cache_ = entity_manager_.get_all_players();
        mob_manager_.set_player_list(&player_list_cache_);
    }

    // Update chunks for all players based on movement
    for (auto& client : clients_) {
        if (client->is_connected() && client->get_state() == SessionState::Play && client->get_player()) {
            const Player* player = client->get_player();
            chunk_streaming_manager_.update_player_chunks(
                client.get(),
                player->get_x(),
                player->get_z()
            );
        }
    }

    // Natural mob spawning
    if (mob_manager_.get_spawner()) {
        mob_manager_.get_spawner()->tick(player_list_cache_);
    }

    // Item entity tick (physics, aging, despawn)
    item_entity_manager_.tick();

    // Item pickup collision checking
    item_entity_manager_.check_pickups(player_list_cache_);
}

void NetworkManager::broadcast_chat(const std::string& message, const std::string& sender) {
    // Format: <sender> message
    std::string formatted = "<" + sender + "> " + message;

    PacketChat chat_packet(formatted);

    // Send to all connected clients in Play state
    for (auto& client : clients_) {
        if (client->is_connected() && client->get_state() == SessionState::Play) {
            client->send_packet(chat_packet);
        }
    }
}

void NetworkManager::broadcast_player_join(const std::string& username) {
    std::string message = "§e" + username + " joined the game";
    PacketChat chat_packet(message);

    // Send to all connected clients in Play state
    for (auto& client : clients_) {
        if (client->is_connected() && client->get_state() == SessionState::Play) {
            client->send_packet(chat_packet);
        }
    }

    LOG_INFO_CAT(username + " joined the game", LogCategory::General);
}

void NetworkManager::broadcast_player_leave(const std::string& username) {
    std::string message = "§e" + username + " left the game";
    PacketChat chat_packet(message);

    // Send to all connected clients in Play state
    for (auto& client : clients_) {
        if (client->is_connected() && client->get_state() == SessionState::Play) {
            client->send_packet(chat_packet);
        }
    }

    LOG_INFO_CAT(username + " left the game", LogCategory::General);
}

void NetworkManager::accept_connections() {
    // Try to accept new connections (non-blocking)
    while (true) {
        auto socket_result = listener_.accept();

        if (!socket_result) {
            // No more pending connections (or error)
            break;
        }

        // Create callbacks for this client
        auto chat_callback = [this](const std::string& message, const std::string& sender) {
            this->broadcast_chat(message, sender);
        };

        auto join_callback = [this](const std::string& username) {
            this->broadcast_player_join(username);
        };

        auto leave_callback = [this](const std::string& username) {
            this->broadcast_player_leave(username);
        };

        auto session = std::make_unique<ClientSession>(
            std::move(socket_result.value()),
            chunk_manager_,
            &entity_manager_,
            &block_manager_,
            &mob_manager_,
            &item_entity_manager_,
            &chunk_streaming_manager_,
            &player_data_manager_,
            &admin_manager_,
            chat_callback,
            join_callback,
            leave_callback
        );
        clients_.push_back(std::move(session));

        LOG_INFO_CAT("Client connected", LogCategory::Network);
    }
}

void NetworkManager::process_clients() {
    // Process all clients and remove disconnected ones
    auto it = clients_.begin();
    while (it != clients_.end()) {
        (*it)->process();

        if (!(*it)->is_connected()) {
            it = clients_.erase(it);
        } else {
            ++it;
        }
    }
}

void NetworkManager::spawn_player_to_client(ClientSession* viewer, const Player* player) {
    if (!viewer || !player) {
        return;
    }

    // Convert player position to fixed-point (multiply by 32)
    i32 fixed_x = static_cast<i32>(std::round(player->get_x() * 32.0));
    i32 fixed_y = static_cast<i32>(std::round(player->get_y() * 32.0));
    i32 fixed_z = static_cast<i32>(std::round(player->get_z() * 32.0));

    // Convert rotation to byte format (angle * 256 / 360)
    i8 yaw = static_cast<i8>(std::round(player->get_yaw() * 256.0f / 360.0f));
    i8 pitch = static_cast<i8>(std::round(player->get_pitch() * 256.0f / 360.0f));

    // Create and send NamedEntitySpawn packet
    PacketNamedEntitySpawn spawn_packet(
        player->get_entity_id(),
        player->get_username(),
        fixed_x, fixed_y, fixed_z,
        yaw, pitch,
        0 // current item (held item ID)
    );

    viewer->send_packet(spawn_packet);

    LOG_DEBUG_CAT("Spawned player " + player->get_username() +
                  " (entity ID " + std::to_string(player->get_entity_id()) + ") " +
                  "to " + viewer->get_username(),
                  LogCategory::Entity);
}

void NetworkManager::despawn_entity_from_client(ClientSession* viewer, i32 entity_id) {
    if (!viewer) {
        return;
    }

    PacketDestroyEntity destroy_packet(entity_id);
    viewer->send_packet(destroy_packet);

    LOG_DEBUG_CAT("Despawned entity ID " + std::to_string(entity_id) +
                  " from " + viewer->get_username(),
                  LogCategory::Entity);
}

void NetworkManager::broadcast_block_change(i32 x, i8 y, i32 z, u8 block_type, u8 metadata) {
    PacketBlockChange block_packet(x, y, z, block_type, metadata);

    // Send to all connected clients in Play state
    for (auto& client : clients_) {
        if (client->is_connected() && client->get_state() == SessionState::Play) {
            client->send_packet(block_packet);
        }
    }

    LOG_DEBUG_CAT("Broadcast block change at (" + std::to_string(x) + ", " +
                  std::to_string(y) + ", " + std::to_string(z) +
                  ") type: " + std::to_string(block_type),
                  LogCategory::World);
}

void NetworkManager::broadcast_chunk_update(i32 chunk_x, i32 chunk_z) {
    // Get the chunk from chunk manager
    Chunk* chunk = chunk_manager_->get_chunk(chunk_x, chunk_z);
    if (!chunk) {
        LOG_WARNING_CAT("Cannot broadcast chunk update - chunk (" + std::to_string(chunk_x) +
                       ", " + std::to_string(chunk_z) + ") not loaded",
                       LogCategory::World);
        return;
    }

    // Create MapChunk packet with updated lighting data
    PacketMapChunk chunk_packet(chunk_x * 16, chunk_z * 16);
    chunk_packet.set_chunk_data(
        chunk->get_blocks_data(),
        chunk->get_metadata_data(),
        chunk->get_block_light_data(),
        chunk->get_sky_light_data()
    );

    // Send to all connected clients in Play state
    for (auto& client : clients_) {
        if (client->is_connected() && client->get_state() == SessionState::Play) {
            client->send_packet(chunk_packet);
        }
    }

    LOG_DEBUG_CAT("Broadcast chunk update for chunk (" + std::to_string(chunk_x) +
                  ", " + std::to_string(chunk_z) + ")",
                  LogCategory::World);
}

void NetworkManager::broadcast_mob_spawn(const Mob* mob) {
    if (!mob) {
        return;
    }

    PacketMobSpawn spawn_packet(mob);

    // Send to all connected clients in Play state
    for (auto& client : clients_) {
        if (client->is_connected() && client->get_state() == SessionState::Play) {
            client->send_packet(spawn_packet);
        }
    }

    LOG_DEBUG_CAT("Broadcast mob spawn: " + mob->get_name() + " (ID: " +
                  std::to_string(mob->get_entity_id()) + ")",
                  LogCategory::Entity);
}

void NetworkManager::broadcast_mob_despawn(i32 entity_id) {
    PacketDestroyEntity destroy_packet(entity_id);

    // Send to all connected clients in Play state
    for (auto& client : clients_) {
        if (client->is_connected() && client->get_state() == SessionState::Play) {
            client->send_packet(destroy_packet);
        }
    }

    LOG_DEBUG_CAT("Broadcast mob despawn (ID: " + std::to_string(entity_id) + ")",
                  LogCategory::Entity);
}

void NetworkManager::broadcast_mob_movement(i32 entity_id, f64 old_x, f64 old_y, f64 old_z,
                                            f64 new_x, f64 new_y, f64 new_z, f32 yaw, f32 pitch) {
    // Calculate relative movement in fixed-point (multiply by 32)
    i8 dx = static_cast<i8>(std::floor((new_x - old_x) * 32.0));
    i8 dy = static_cast<i8>(std::floor((new_y - old_y) * 32.0));
    i8 dz = static_cast<i8>(std::floor((new_z - old_z) * 32.0));

    // Convert yaw/pitch to byte format
    i8 yaw_byte = static_cast<i8>(static_cast<i32>(yaw * 256.0f / 360.0f) & 0xFF);
    i8 pitch_byte = static_cast<i8>(static_cast<i32>(pitch * 256.0f / 360.0f) & 0xFF);

    // Use combined packet if both movement and rotation occurred
    PacketEntityLookMove move_packet(entity_id, dx, dy, dz, yaw_byte, pitch_byte);

    // Send to all connected clients in Play state
    for (auto& client : clients_) {
        if (client->is_connected() && client->get_state() == SessionState::Play) {
            client->send_packet(move_packet);
        }
    }
}

void NetworkManager::send_health_update(i32 entity_id, i16 health) {
    // Find the player's session and send health update
    Player* player = entity_manager_.get_player(entity_id);
    if (!player) {
        return;
    }

    // Find the client session for this player
    for (auto& client : clients_) {
        if (client->is_connected() &&
            client->get_state() == SessionState::Play &&
            client->get_player() &&
            client->get_player()->get_entity_id() == entity_id) {

            PacketUpdateHealth health_packet(health);
            client->send_packet(health_packet);

            LOG_DEBUG_CAT("Sent health update to " + player->get_username() +
                         ": " + std::to_string(health) + "/20 HP",
                         LogCategory::Entity);
            break;
        }
    }
}

void NetworkManager::broadcast_entity_status(i32 entity_id, i8 status) {
    PacketEntityStatus status_packet(entity_id, status);

    // Broadcast to all connected clients in Play state
    for (auto& client : clients_) {
        if (client->is_connected() && client->get_state() == SessionState::Play) {
            client->send_packet(status_packet);
        }
    }

    const char* status_str = (status == 2) ? "hurt" : (status == 3) ? "dead" : "unknown";
    LOG_DEBUG_CAT("Broadcast entity status for entity " + std::to_string(entity_id) +
                 ": " + status_str,
                 LogCategory::Entity);
}

void NetworkManager::handle_player_death(i32 entity_id) {
    Player* player = entity_manager_.get_player(entity_id);
    if (!player) {
        return;
    }

    LOG_INFO_CAT(player->get_username() + " died! Respawning...", LogCategory::Entity);

    // Spawn position (world spawn - should be configurable)
    constexpr f64 spawn_x = 0.0;
    constexpr f64 spawn_y = 64.0;
    constexpr f64 spawn_z = 0.0;

    // Respawn the player
    player->respawn(spawn_x, spawn_y, spawn_z);

    // Find the client session and send respawn packets
    for (auto& client : clients_) {
        if (client->is_connected() &&
            client->get_state() == SessionState::Play &&
            client->get_player() &&
            client->get_player()->get_entity_id() == entity_id) {

            // Send Respawn packet (resets client state)
            PacketRespawn respawn_packet(
                0,      // dimension: overworld
                1,      // difficulty: easy (should match server difficulty)
                0,      // creative_mode: survival
                128,    // world_height: always 128 in Beta 1.7.3
                0       // map_seed: TODO - get actual seed
            );
            client->send_packet(respawn_packet);

            // Send PlayerPositionLook to move player to spawn
            PacketPlayerPositionLook pos_packet;
            pos_packet.x = spawn_x;
            pos_packet.y = spawn_y + 1.62;  // Add eye height
            pos_packet.stance = spawn_y + 1.62;
            pos_packet.z = spawn_z;
            pos_packet.yaw = 0.0f;
            pos_packet.pitch = 0.0f;
            pos_packet.on_ground = false;
            client->send_packet(pos_packet);

            LOG_INFO_CAT(player->get_username() + " respawned at spawn point", LogCategory::Entity);
            break;
        }
    }
}

void NetworkManager::broadcast_item_spawn(const ItemEntity* item) {
    if (!item) {
        return;
    }

    // PacketPickupSpawn constructor handles fixed-point conversion
    PacketPickupSpawn spawn_packet(
        item->get_entity_id(),
        item->get_item()->get_item_id(),
        item->get_item()->get_count(),
        item->get_item()->get_damage(),
        item->get_x(),  // Pass raw f64 coordinates
        item->get_y(),
        item->get_z(),
        0, 0, 0  // rotation, pitch, roll
    );

    // Send to all connected clients in Play state
    for (auto& client : clients_) {
        if (client->is_connected() && client->get_state() == SessionState::Play) {
            client->send_packet(spawn_packet);
        }
    }

    LOG_DEBUG_CAT("Broadcast item spawn (entity ID: " + std::to_string(item->get_entity_id()) +
                  ", item ID: " + std::to_string(item->get_item()->get_item_id()) + ")",
                  LogCategory::Entity);
}

void NetworkManager::broadcast_item_despawn(i32 entity_id) {
    PacketDestroyEntity destroy_packet(entity_id);

    // Send to all connected clients in Play state
    for (auto& client : clients_) {
        if (client->is_connected() && client->get_state() == SessionState::Play) {
            client->send_packet(destroy_packet);
        }
    }

    LOG_DEBUG_CAT("Broadcast item despawn (entity ID: " + std::to_string(entity_id) + ")",
                  LogCategory::Entity);
}

void NetworkManager::broadcast_item_collect(i32 item_entity_id, i32 collector_entity_id) {
    PacketCollect collect_packet(item_entity_id, collector_entity_id);

    // Send to all connected clients in Play state
    ClientSession* collector_session = nullptr;
    for (auto& client : clients_) {
        if (client->is_connected() && client->get_state() == SessionState::Play) {
            client->send_packet(collect_packet);

            // Track which client is the collector
            if (client->get_player() && client->get_player()->get_entity_id() == collector_entity_id) {
                collector_session = client.get();
            }
        }
    }

    // Send inventory update to the collector so they see the item in their inventory
    if (collector_session) {
        collector_session->send_full_inventory();
    }

    LOG_DEBUG_CAT("Broadcast item collect: item entity " + std::to_string(item_entity_id) +
                  " collected by entity " + std::to_string(collector_entity_id),
                  LogCategory::Entity);
}

} // namespace mcserver
