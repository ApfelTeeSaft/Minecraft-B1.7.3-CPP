#include "socket.hpp"
#include <cstring>

#ifdef PLATFORM_WINDOWS
#pragma comment(lib, "ws2_32.lib")
#else
#include <sys/socket.h>
#include <netinet/in.h>
#include <netinet/tcp.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <fcntl.h>
#include <errno.h>
#endif

namespace mcserver {

#ifdef PLATFORM_WINDOWS
static WSADATA g_wsa_data;
static bool g_wsa_initialized = false;
#endif

Result<void> init_networking() {
#ifdef PLATFORM_WINDOWS
    if (!g_wsa_initialized) {
        int result = WSAStartup(MAKEWORD(2, 2), &g_wsa_data);
        if (result != 0) {
            return ErrorCode::NetworkError;
        }
        g_wsa_initialized = true;
    }
#endif
    return Result<void>();
}

void shutdown_networking() {
#ifdef PLATFORM_WINDOWS
    if (g_wsa_initialized) {
        WSACleanup();
        g_wsa_initialized = false;
    }
#endif
}

Socket::Socket() : socket_(INVALID_SOCKET_VALUE) {}

Socket::Socket(socket_t native_socket) : socket_(native_socket) {}

Socket::~Socket() {
    close();
}

Socket::Socket(Socket&& other) noexcept : socket_(other.socket_) {
    other.socket_ = INVALID_SOCKET_VALUE;
}

Socket& Socket::operator=(Socket&& other) noexcept {
    if (this != &other) {
        close();
        socket_ = other.socket_;
        other.socket_ = INVALID_SOCKET_VALUE;
    }
    return *this;
}

Result<Socket> Socket::create_tcp() {
    socket_t sock = ::socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    if (sock == INVALID_SOCKET_VALUE) {
        return get_last_socket_error();
    }
    return Socket(sock);
}

Result<void> Socket::bind(std::string_view address, u16 port) {
    if (!is_valid()) {
        return ErrorCode::InvalidArgument;
    }

    sockaddr_in addr{};
    addr.sin_family = AF_INET;
    addr.sin_port = htons(port);

    if (address.empty() || address == "*") {
        addr.sin_addr.s_addr = INADDR_ANY;
    } else {
        if (inet_pton(AF_INET, std::string(address).c_str(), &addr.sin_addr) <= 0) {
            return ErrorCode::InvalidArgument;
        }
    }

    if (::bind(socket_, reinterpret_cast<sockaddr*>(&addr), sizeof(addr)) < 0) {
        return get_last_socket_error();
    }

    return Result<void>();
}

Result<void> Socket::listen(i32 backlog) {
    if (!is_valid()) {
        return ErrorCode::InvalidArgument;
    }

    if (::listen(socket_, backlog) < 0) {
        return get_last_socket_error();
    }

    return Result<void>();
}

Result<Socket> Socket::accept() {
    if (!is_valid()) {
        return ErrorCode::InvalidArgument;
    }

    sockaddr_in client_addr{};
    socklen_t client_len = sizeof(client_addr);

    socket_t client_sock = ::accept(socket_, reinterpret_cast<sockaddr*>(&client_addr), &client_len);
    if (client_sock == INVALID_SOCKET_VALUE) {
        return get_last_socket_error();
    }

    return Socket(client_sock);
}

Result<void> Socket::connect(std::string_view address, u16 port) {
    if (!is_valid()) {
        return ErrorCode::InvalidArgument;
    }

    sockaddr_in addr{};
    addr.sin_family = AF_INET;
    addr.sin_port = htons(port);

    if (inet_pton(AF_INET, std::string(address).c_str(), &addr.sin_addr) <= 0) {
        return ErrorCode::InvalidArgument;
    }

    if (::connect(socket_, reinterpret_cast<sockaddr*>(&addr), sizeof(addr)) < 0) {
        return get_last_socket_error();
    }

    return Result<void>();
}

Result<isize> Socket::send(const byte* data, usize size) {
    if (!is_valid()) {
        return ErrorCode::InvalidArgument;
    }

    isize sent = ::send(socket_, reinterpret_cast<const char*>(data), static_cast<int>(size), 0);
    if (sent < 0) {
        return get_last_socket_error();
    }

    return sent;
}

Result<isize> Socket::receive(byte* buffer, usize size) {
    if (!is_valid()) {
        return ErrorCode::InvalidArgument;
    }

    isize received = ::recv(socket_, reinterpret_cast<char*>(buffer), static_cast<int>(size), 0);
    if (received < 0) {
        return get_last_socket_error();
    }

    return received;
}

Result<void> Socket::set_non_blocking(bool enabled) {
    if (!is_valid()) {
        return ErrorCode::InvalidArgument;
    }

#ifdef PLATFORM_WINDOWS
    u_long mode = enabled ? 1 : 0;
    if (ioctlsocket(socket_, FIONBIO, &mode) != 0) {
        return get_last_socket_error();
    }
#else
    int flags = fcntl(socket_, F_GETFL, 0);
    if (flags < 0) {
        return get_last_socket_error();
    }

    flags = enabled ? (flags | O_NONBLOCK) : (flags & ~O_NONBLOCK);
    if (fcntl(socket_, F_SETFL, flags) < 0) {
        return get_last_socket_error();
    }
#endif

    return Result<void>();
}

Result<void> Socket::set_reuse_address(bool enabled) {
    if (!is_valid()) {
        return ErrorCode::InvalidArgument;
    }

    int opt = enabled ? 1 : 0;
    if (setsockopt(socket_, SOL_SOCKET, SO_REUSEADDR, reinterpret_cast<const char*>(&opt), sizeof(opt)) < 0) {
        return get_last_socket_error();
    }

    return Result<void>();
}

Result<void> Socket::set_tcp_nodelay(bool enabled) {
    if (!is_valid()) {
        return ErrorCode::InvalidArgument;
    }

    int opt = enabled ? 1 : 0;
    if (setsockopt(socket_, IPPROTO_TCP, TCP_NODELAY, reinterpret_cast<const char*>(&opt), sizeof(opt)) < 0) {
        return get_last_socket_error();
    }

    return Result<void>();
}

Result<void> Socket::set_send_buffer_size(i32 size) {
    if (!is_valid()) {
        return ErrorCode::InvalidArgument;
    }

    if (setsockopt(socket_, SOL_SOCKET, SO_SNDBUF, reinterpret_cast<const char*>(&size), sizeof(size)) < 0) {
        return get_last_socket_error();
    }

    return Result<void>();
}

Result<void> Socket::set_receive_buffer_size(i32 size) {
    if (!is_valid()) {
        return ErrorCode::InvalidArgument;
    }

    if (setsockopt(socket_, SOL_SOCKET, SO_RCVBUF, reinterpret_cast<const char*>(&size), sizeof(size)) < 0) {
        return get_last_socket_error();
    }

    return Result<void>();
}

void Socket::close() {
    if (is_valid()) {
#ifdef PLATFORM_WINDOWS
        closesocket(socket_);
#else
        ::close(socket_);
#endif
        socket_ = INVALID_SOCKET_VALUE;
    }
}

ErrorCode Socket::get_last_socket_error() {
#ifdef PLATFORM_WINDOWS
    int error = WSAGetLastError();
    switch (error) {
        case WSAEWOULDBLOCK:
        case WSAEINPROGRESS:
            return ErrorCode::Timeout;
        case WSAEACCES:
            return ErrorCode::PermissionDenied;
        default:
            return ErrorCode::NetworkError;
    }
#else
    switch (errno) {
        case EWOULDBLOCK:
        case EINPROGRESS:
            return ErrorCode::Timeout;
        case EACCES:
            return ErrorCode::PermissionDenied;
        case ECONNREFUSED:
            return ErrorCode::NetworkError;
        default:
            return ErrorCode::NetworkError;
    }
#endif
}

} // namespace mcserver
