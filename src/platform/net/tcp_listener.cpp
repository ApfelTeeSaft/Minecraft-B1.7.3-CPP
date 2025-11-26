#include "tcp_listener.hpp"
#include "util/log/logger.hpp"

namespace mcserver {

TcpListener::~TcpListener() {
    stop();
}

Result<void> TcpListener::start(std::string_view address, u16 port, i32 backlog) {
    if (is_listening()) {
        return ErrorCode::AlreadyExists;
    }

    auto socket_result = Socket::create_tcp();
    if (!socket_result) {
        return socket_result.error();
    }

    listen_socket_ = std::move(socket_result.value());

    // Set socket options
    listen_socket_.set_reuse_address(true);
    listen_socket_.set_non_blocking(true);

    // Bind to address
    auto bind_result = listen_socket_.bind(address, port);
    if (!bind_result) {
        listen_socket_.close();
        return bind_result.error();
    }

    // Start listening
    auto listen_result = listen_socket_.listen(backlog);
    if (!listen_result) {
        listen_socket_.close();
        return listen_result.error();
    }

    return Result<void>();
}

void TcpListener::stop() {
    listen_socket_.close();
}

Result<Socket> TcpListener::accept() {
    if (!is_listening()) {
        return ErrorCode::InvalidArgument;
    }

    return listen_socket_.accept();
}

} // namespace mcserver
