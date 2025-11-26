#pragma once

#include "platform/net/tcp_listener.hpp"
#include "net/session/client_session.hpp"
#include "net/transport/chunk_streaming_manager.hpp"
#include "entity/entity_manager.hpp"
#include "entity/mob/mob_manager.hpp"
#include "entity/item/item_entity_manager.hpp"
#include "world/block/block_manager.hpp"
#include "storage/player/player_data_manager.hpp"
#include "storage/async/async_io.hpp"
#include "core/scheduler/job_system.hpp"
#include "admin/admin_manager.hpp"
#include "util/result.hpp"
#include <vector>
#include <memory>
#include <string>

namespace mcserver {

class ChunkManager;
class Player;
class Mob;
class ItemEntity;

class NetworkManager {
public:
    NetworkManager(ChunkManager* chunk_manager, const std::string& world_path);
    ~NetworkManager();

    // Start listening for connections
    Result<void> start(const std::string& address, u16 port);

    // Stop listening
    void stop();

    // Process network I/O (accept connections, process packets)
    void tick();

    // Get connected client count
    usize client_count() const { return clients_.size(); }

    // Broadcast a chat message to all clients
    void broadcast_chat(const std::string& message, const std::string& sender);

    // Broadcast player join message
    void broadcast_player_join(const std::string& username);

    // Broadcast player leave message
    void broadcast_player_leave(const std::string& username);

    // Get entity manager
    EntityManager* get_entity_manager() { return &entity_manager_; }

    // Get block manager
    BlockManager* get_block_manager() { return &block_manager_; }

    // Get mob manager
    MobManager* get_mob_manager() { return &mob_manager_; }

    // Get item entity manager
    ItemEntityManager* get_item_entity_manager() { return &item_entity_manager_; }

    // Get chunk streaming manager
    ChunkStreamingManager* get_chunk_streaming_manager() { return &chunk_streaming_manager_; }

    // Get player data manager
    PlayerDataManager* get_player_data_manager() { return &player_data_manager_; }

    // Broadcast a block change to all nearby clients
    void broadcast_block_change(i32 x, i8 y, i32 z, u8 block_type, u8 metadata);

    // Broadcast a chunk update (resend chunk data for lighting updates)
    void broadcast_chunk_update(i32 chunk_x, i32 chunk_z);

    // Broadcast a mob spawn to all clients
    void broadcast_mob_spawn(const Mob* mob);

    // Broadcast a mob despawn to all clients
    void broadcast_mob_despawn(i32 entity_id);

    // Broadcast mob movement to all clients
    void broadcast_mob_movement(i32 entity_id, f64 old_x, f64 old_y, f64 old_z,
                                f64 new_x, f64 new_y, f64 new_z, f32 yaw, f32 pitch);

    // Broadcast player health update to specific player
    void send_health_update(i32 entity_id, i16 health);

    // Broadcast entity status (damage/death animation) to all nearby players
    void broadcast_entity_status(i32 entity_id, i8 status);

    // Handle player death and respawn
    void handle_player_death(i32 entity_id);

    // Broadcast item entity spawn to all clients
    void broadcast_item_spawn(const ItemEntity* item);

    // Broadcast item entity despawn to all clients
    void broadcast_item_despawn(i32 entity_id);

    // Broadcast item collect to all clients
    void broadcast_item_collect(i32 item_entity_id, i32 collector_entity_id);

private:
    ChunkManager* chunk_manager_;
    JobSystem job_system_;
    AsyncIO async_io_;
    EntityManager entity_manager_;
    BlockManager block_manager_;
    MobManager mob_manager_;
    ItemEntityManager item_entity_manager_;
    ChunkStreamingManager chunk_streaming_manager_;
    PlayerDataManager player_data_manager_;
    AdminManager admin_manager_;
    TcpListener listener_;
    std::vector<std::unique_ptr<ClientSession>> clients_;
    std::vector<Player*> player_list_cache_;  // Cached player list for mob targeting

    void accept_connections();
    void process_clients();

    // Entity spawn/despawn callbacks
    void spawn_player_to_client(ClientSession* viewer, const Player* player);
    void despawn_entity_from_client(ClientSession* viewer, i32 entity_id);
};

} // namespace mcserver
