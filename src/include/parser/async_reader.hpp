#pragma once
#include <string>
#include <boost/asio.hpp>

class AsyncReader {
    boost::asio::posix::stream_descriptor stream_;
    boost::asio::streambuf buffer_;

public:
    AsyncReader(boost::asio::io_context& ctx, int fd)
        : stream_(ctx, fd) {}

    boost::asio::awaitable<std::string> read_line() {
        co_await boost::asio::async_read_until(
            stream_, buffer_, '\n', boost::asio::use_awaitable);

        std::istream is(&buffer_);
        std::string line;
        std::getline(is, line);

        co_return line;
    }
};
