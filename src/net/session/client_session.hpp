#pragma once

#include "platform/net/socket.hpp"
#include "net/protocol/packet.hpp"
#include "entity/player.hpp"
#include "util/result.hpp"
#include <vector>
#include <memory>
#include <string>
#include <functional>

namespace mcserver {

class ChunkManager;
class EntityManager;
class BlockManager;
class MobManager;
class ItemEntityManager;
class ChunkStreamingManager;
class PlayerDataManager;
class AdminManager;

enum class SessionState {
    Handshake,
    Login,
    Play,
    Disconnected
};

// Callback type for broadcasting chat messages
using ChatBroadcastCallback = std::function<void(const std::string& message, const std::string& sender)>;
using PlayerJoinCallback = std::function<void(const std::string& username)>;
using PlayerLeaveCallback = std::function<void(const std::string& username)>;

class ClientSession {
public:
    explicit ClientSession(Socket socket, ChunkManager* chunk_manager,
                          EntityManager* entity_manager,
                          BlockManager* block_manager,
                          MobManager* mob_manager,
                          ItemEntityManager* item_entity_manager,
                          ChunkStreamingManager* chunk_streaming_manager,
                          PlayerDataManager* player_data_manager,
                          AdminManager* admin_manager,
                          ChatBroadcastCallback chat_callback,
                          PlayerJoinCallback join_callback,
                          PlayerLeaveCallback leave_callback);
    ~ClientSession();

    // Process incoming data
    void process();

    // Send packet
    void send_packet(const Packet& packet);

    // Disconnect client
    void disconnect(const std::string& reason = "");

    // Getters
    bool is_connected() const { return state_ != SessionState::Disconnected; }
    SessionState get_state() const { return state_; }
    const std::string& get_username() const { return username_; }
    Player* get_player() { return player_.get(); }
    const Player* get_player() const { return player_.get(); }

    // Public inventory sync method for external callers (e.g., NetworkManager for item pickup)
    void send_full_inventory();

private:
    Socket socket_;
    ChunkManager* chunk_manager_;
    EntityManager* entity_manager_;
    BlockManager* block_manager_;
    MobManager* mob_manager_;
    ItemEntityManager* item_entity_manager_;
    ChunkStreamingManager* chunk_streaming_manager_;
    PlayerDataManager* player_data_manager_;
    AdminManager* admin_manager_;
    ChatBroadcastCallback chat_callback_;
    PlayerJoinCallback join_callback_;
    PlayerLeaveCallback leave_callback_;
    SessionState state_;
    std::string username_;
    std::unique_ptr<Player> player_;
    std::vector<byte> recv_buffer_;
    std::vector<byte> send_buffer_;

    void handle_handshake(PacketBuffer& buffer);
    void handle_login(PacketBuffer& buffer);
    bool handle_play_packet(u8 packet_id, PacketBuffer& buffer);

    // Helper to send initial chunks after login
    void send_initial_chunks();

    // Helper to send single slot update to client
    void send_inventory_update(i32 slot);

    // Handle chat commands
    void handle_command(const std::string& command);
    void send_chat_message(const std::string& message);
};

} // namespace mcserver
