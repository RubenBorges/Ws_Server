#include <web/server.hpp>

#include <boost/asio/redirect_error.hpp>
#include <boost/asio/use_awaitable.hpp>

#include <istream>
#include <string>
#include <utility>

namespace p2p::web {

AsyncEchoServer::AsyncEchoServer(asio::io_context& io, ServerOptions options)
    : io_(io), acceptor_(io), options_(std::move(options)) {
    acceptor_.open(tcp::v4());
    acceptor_.set_option(asio::socket_base::reuse_address(true));
    acceptor_.bind(tcp::endpoint(tcp::v4(), options_.port));
    acceptor_.listen(asio::socket_base::max_listen_connections);
}

void AsyncEchoServer::start() {
    auto self = shared_from_this();
    asio::co_spawn(io_, [self]() { return self->accept_loop(); }, asio::detached);
}

void AsyncEchoServer::request_stop() {
    if (stopped_.exchange(true)) {
        return;
    }

    try {
        acceptor_.cancel();
    } catch (...) {
    }
    try {
        acceptor_.close();
    } catch (...) {
    }
    io_.stop();
}

bool AsyncEchoServer::is_stopped() const noexcept {
    return stopped_.load();
}

asio::awaitable<void> AsyncEchoServer::accept_loop() {
    while (!is_stopped()) {
        boost::system::error_code ec;
        tcp::socket socket =
            co_await acceptor_.async_accept(asio::redirect_error(asio::use_awaitable, ec));

        if (ec) {
            if (is_stopped() || ec == asio::error::operation_aborted) {
                break;
            }
            continue;
        }

        auto self = shared_from_this();
        asio::co_spawn(io_,
                       [self, socket = std::move(socket)]() mutable {
                           return self->session_loop(std::move(socket));
                       },
                       asio::detached);
    }
}

asio::awaitable<void> AsyncEchoServer::session_loop(tcp::socket socket) {
    asio::streambuf read_buffer;

    while (!is_stopped()) {
        boost::system::error_code ec;
        co_await asio::async_read_until(
            socket, read_buffer, '\n', asio::redirect_error(asio::use_awaitable, ec));

        if (ec) {
            break;
        }

        std::istream input(&read_buffer);
        std::string line;
        std::getline(input, line);

        if (line == options_.termination_command) {
            const std::string shutdown_ack = "server shutting down\n";
            co_await asio::async_write(
                socket, asio::buffer(shutdown_ack), asio::redirect_error(asio::use_awaitable, ec));
            request_stop();
            break;
        }

        line.push_back('\n');
        co_await asio::async_write(
            socket, asio::buffer(line), asio::redirect_error(asio::use_awaitable, ec));

        if (ec) {
            break;
        }
    }
}

std::shared_ptr<AsyncEchoServer> make_server(asio::io_context& io, ServerOptions options) {
    return std::make_shared<AsyncEchoServer>(io, std::move(options));
}

}  // namespace p2p::web