#include <web/server.hpp>

#include <boost/asio/redirect_error.hpp>
#include <boost/asio/use_awaitable.hpp>
#include <boost/beast/websocket.hpp> 
#include <web/router.hpp>           
#include <iostream>
#include <string>
#include <utility>

namespace p2p::web {

// Constructs the server object, preparing the acceptor and storing the server options.
AsyncEchoServer::AsyncEchoServer(asio::io_context& io, ServerOptions options)
    : io_(io), acceptor_(io), options_(std::move(options)) {
    acceptor_.open(tcp::v4());
    acceptor_.set_option(asio::socket_base::reuse_address(true));
    acceptor_.bind(tcp::endpoint(tcp::v4(), options_.port));
    acceptor_.listen(asio::socket_base::max_listen_connections);
}

// Begins accepting client connections by spawning the accept loop coroutine.
void AsyncEchoServer::start() {
    auto self = shared_from_this();
    asio::co_spawn(io_, [self]() { return self->accept_loop(); }, asio::detached);
}

// Requests shutdown by canceling the acceptor and stopping the I/O context.
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

// Returns whether the server has already been told to stop.
bool AsyncEchoServer::is_stopped() const noexcept {
    return stopped_.load();
}

// Continuously waits for new incoming connections and spawns a session for each one.
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

// Handles a single client connection by upgrading it to a WebSocket connection
asio::awaitable<void> AsyncEchoServer::session_loop(tcp::socket socket) {
    boost::system::error_code ec;

    // 1. Upgrade the raw incoming TCP socket to a Boost.Beast WebSocket stream
    ws_stream ws(std::move(socket));

    // 2. Perform the async WebSocket handshake
    co_await ws.async_accept(asio::redirect_error(asio::use_awaitable, ec));
    if (ec) {
        std::cerr << "WebSocket handshake failed: " << ec.message() << "\n";
        co_return;
    }

    // 3. Allocate a dynamic buffer to hold raw incoming packet frames
    boost::beast::flat_buffer read_buffer;

    while (!is_stopped()) {
        read_buffer.clear();

        // 4. Read the next incoming frame asynchronously
        co_await ws.async_read(read_buffer, asio::redirect_error(asio::use_awaitable, ec));
        if (ec) {
            if (ec == boost::beast::websocket::error::closed) {
                std::cout << "Client gracefully closed connection.\n";
            } else {
                std::cerr << "WebSocket read error: " << ec.message() << "\n";
            }
            break; 
        }

        // 5. Convert incoming payload buffer bytes into a string command
        std::string incoming_msg(boost::asio::buffers_begin(read_buffer.data()),
                                 boost::asio::buffers_end(read_buffer.data()));

        // Check for termination command frame
        if (incoming_msg == options_.termination_command) {
            std::cout << "Termination command received. Shutting down server...\n";
            request_stop();
            break;
        }

        // 6. Setup transaction context and fire it down into your variant router
        // NOTE: Here we pass a mockup layout. Later, you can populate ThisDataTransaction 
        // with fields parsed directly out of incoming_msg.
        RequestVariant currentPayload = ThisDataTransaction; 
        
        // Dispatches the payload through the variant lookup matrix table
        dispatch("data", currentPayload); 

        // 7. Extract the modified payload data transaction state back out 
        if (auto* txPtr = std::get_if<data::dataTransaction>(&currentPayload)) {
            // Send the raw data transaction buffer right back down this specific WebSocket link
            data_ops::sendBinaryToWebSocket(ws, *txPtr);
        }
    }
}

// Creates a server instance for the caller using the provided I/O context and options.
std::shared_ptr<AsyncEchoServer> make_server(asio::io_context& io, ServerOptions options) {
    return std::make_shared<AsyncEchoServer>(io, std::move(options));
}

}  // namespace p2p::web
