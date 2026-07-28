#include <web/client.hpp>

#include <boost/asio/redirect_error.hpp>
#include <boost/asio/use_awaitable.hpp>

#include <istream>
#include <stdexcept>
#include <string>

namespace p2p::web {

AsyncEchoClient::AsyncEchoClient(asio::io_context& io)
  : io_(io), resolver_(io), socket_(io) {}

asio::awaitable<void> AsyncEchoClient::connect(const ClientOptions& options) {
  auto endpoints = co_await resolver_.async_resolve(
    options.host, std::to_string(options.port), asio::use_awaitable);

  boost::system::error_code ec;
  co_await asio::async_connect(socket_, endpoints, asio::redirect_error(asio::use_awaitable, ec));
  if (ec) {
    throw std::runtime_error("client connect failed: " + ec.message());
  }
}

asio::awaitable<void> AsyncEchoClient::send_line(const std::string& line) {
  std::string payload = line;
  payload.push_back('\n');

  boost::system::error_code ec;
  co_await asio::async_write(
    socket_, asio::buffer(payload), asio::redirect_error(asio::use_awaitable, ec));
  if (ec) {
    throw std::runtime_error("client write failed: " + ec.message());
  }
}

asio::awaitable<std::string> AsyncEchoClient::read_line() {
  boost::system::error_code ec;
  co_await asio::async_read_until(
    socket_, read_buffer_, '\n', asio::redirect_error(asio::use_awaitable, ec));
  if (ec) {
    throw std::runtime_error("client read failed: " + ec.message());
  }

  std::istream input(&read_buffer_);
  std::string line;
  std::getline(input, line);
  co_return line;
}

asio::awaitable<std::string> AsyncEchoClient::send_and_receive(const std::string& line) {
  co_await send_line(line);
  co_return co_await read_line();
}

void AsyncEchoClient::close() {
  try {
    socket_.shutdown(tcp::socket::shutdown_both);
  } catch (...) {
  }
  try {
    socket_.close();
  } catch (...) {
  }
}

bool AsyncEchoClient::is_open() const {
  return socket_.is_open();
}

}  // namespace p2p::web