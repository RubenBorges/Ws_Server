#pragma once
#include <boost/asio.hpp>
#include <string>

namespace p2p::web {

namespace asio = boost::asio;
using tcp = asio::ip::tcp;

struct ClientOptions {
    std::string host;
    unsigned short port;
};

class AsyncEchoClient {
public:
    explicit AsyncEchoClient(asio::io_context& io);

    asio::awaitable<void> connect(const ClientOptions& options);
    asio::awaitable<void> send_line(const std::string& line);
    asio::awaitable<std::string> read_line();
    asio::awaitable<std::string> send_and_receive(const std::string& line);

    void close();
    bool is_open() const;

private:
    asio::io_context& io_;
    tcp::resolver resolver_;
    tcp::socket socket_;
    asio::streambuf read_buffer_;
};

}  // namespace p2p::web