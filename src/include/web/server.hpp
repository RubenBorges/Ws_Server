#pragma once
#include <boost/asio.hpp>
#include <atomic>
#include <memory>
#include <string>
#include <web/router.hpp>
namespace p2p::web {

namespace asio = boost::asio;
using tcp = asio::ip::tcp;

struct ServerOptions {
	unsigned short port;
	std::string termination_command = "shutdown";
};

class AsyncEchoServer : public std::enable_shared_from_this<AsyncEchoServer> {
public:
	AsyncEchoServer(asio::io_context& io, ServerOptions options);

	void start();
	void request_stop();
	bool is_stopped() const noexcept;

private:
	asio::awaitable<void> accept_loop();
	asio::awaitable<void> session_loop(tcp::socket socket);
	asio::io_context& io_;
	tcp::acceptor acceptor_;
	ServerOptions options_;
	std::atomic<bool> stopped_{false};
};

std::shared_ptr<AsyncEchoServer> make_server(asio::io_context& io, ServerOptions options) ;
}  // namespace p2p::web