#pragma once

#include "util/types.hpp"
#include "util/result.hpp"
#include <string>
#include <string_view>

#ifdef PLATFORM_WINDOWS
#include <winsock2.h>
#include <ws2tcpip.h>
using socket_t = SOCKET;
#define INVALID_SOCKET_VALUE INVALID_SOCKET
#else
using socket_t = int;
#define INVALID_SOCKET_VALUE (-1)
#endif

namespace mcserver {

class Socket {
public:
    Socket();
    explicit Socket(socket_t native_socket);
    ~Socket();

    Socket(const Socket&) = delete;
    Socket& operator=(const Socket&) = delete;

    Socket(Socket&& other) noexcept;
    Socket& operator=(Socket&& other) noexcept;

    // Create a TCP socket
    static Result<Socket> create_tcp();

    // Bind to address and port
    Result<void> bind(std::string_view address, u16 port);

    // Listen for connections
    Result<void> listen(i32 backlog = 128);

    // Accept a connection
    Result<Socket> accept();

    // Connect to remote address
    Result<void> connect(std::string_view address, u16 port);

    // Send data
    Result<isize> send(const byte* data, usize size);

    // Receive data
    Result<isize> receive(byte* buffer, usize size);

    // Set non-blocking mode
    Result<void> set_non_blocking(bool enabled);

    // Set socket options
    Result<void> set_reuse_address(bool enabled);
    Result<void> set_tcp_nodelay(bool enabled);
    Result<void> set_send_buffer_size(i32 size);
    Result<void> set_receive_buffer_size(i32 size);

    // Close the socket
    void close();

    // Check if socket is valid
    bool is_valid() const { return socket_ != INVALID_SOCKET_VALUE; }

    // Get native socket handle
    socket_t native_handle() const { return socket_; }

private:
    socket_t socket_;

    static ErrorCode get_last_socket_error();
};

// Initialize platform networking (Windows WSA)
Result<void> init_networking();
void shutdown_networking();

} // namespace mcserver
