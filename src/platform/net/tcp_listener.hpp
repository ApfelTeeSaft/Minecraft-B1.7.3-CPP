#pragma once

#include "socket.hpp"
#include "util/result.hpp"
#include <string>
#include <vector>

namespace mcserver {

class TcpListener {
public:
    TcpListener() = default;
    ~TcpListener();

    TcpListener(const TcpListener&) = delete;
    TcpListener& operator=(const TcpListener&) = delete;

    // Start listening on specified port
    Result<void> start(std::string_view address, u16 port, i32 backlog = 128);

    // Stop listening
    void stop();

    // Accept incoming connections (non-blocking if socket is non-blocking)
    Result<Socket> accept();

    // Check if listening
    bool is_listening() const { return listen_socket_.is_valid(); }

private:
    Socket listen_socket_;
};

} // namespace mcserver
