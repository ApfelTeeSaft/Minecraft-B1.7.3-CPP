#pragma once

#include "event.hpp"

namespace mcserver {

class Server;

// Server enable event (called when server starts)
class ServerEnableEvent : public Event {
public:
    explicit ServerEnableEvent(Server* server) : server_(server) {}

    const char* get_event_name() const override { return "ServerEnableEvent"; }

    Server* get_server() const { return server_; }

private:
    Server* server_;
};

// Server disable event (called when server stops)
class ServerDisableEvent : public Event {
public:
    explicit ServerDisableEvent(Server* server) : server_(server) {}

    const char* get_event_name() const override { return "ServerDisableEvent"; }

    Server* get_server() const { return server_; }

private:
    Server* server_;
};

// Chunk load event
class ChunkLoadEvent : public Event {
public:
    ChunkLoadEvent(i32 chunk_x, i32 chunk_z, bool new_chunk)
        : chunk_x_(chunk_x), chunk_z_(chunk_z), new_chunk_(new_chunk) {}

    const char* get_event_name() const override { return "ChunkLoadEvent"; }

    i32 get_chunk_x() const { return chunk_x_; }
    i32 get_chunk_z() const { return chunk_z_; }

    // True if chunk was newly generated, false if loaded from disk
    bool is_new_chunk() const { return new_chunk_; }

private:
    i32 chunk_x_;
    i32 chunk_z_;
    bool new_chunk_;
};

// Chunk unload event
class ChunkUnloadEvent : public Event {
public:
    ChunkUnloadEvent(i32 chunk_x, i32 chunk_z)
        : chunk_x_(chunk_x), chunk_z_(chunk_z) {}

    const char* get_event_name() const override { return "ChunkUnloadEvent"; }

    i32 get_chunk_x() const { return chunk_x_; }
    i32 get_chunk_z() const { return chunk_z_; }

private:
    i32 chunk_x_;
    i32 chunk_z_;
};

} // namespace mcserver
