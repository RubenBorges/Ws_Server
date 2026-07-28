#pragma once
#include <boost/asio.hpp>
#include <nlohmann/json.hpp>
#include <string>
#include <functional>
#include <memory>
#include <istream>

namespace p2p {

// Synchronous line-by-line JSON parser using istream
class SyncJsonParser {
public:
    // Parse each line as JSON, calling cb for each object
    static void parse(std::istream& in, std::function<void(const nlohmann::json&)> cb) {
        std::string line;
        while (std::getline(in, line)) {
            try {
                auto obj = nlohmann::json::parse(line);
                cb(obj);
            } catch (...) {}
        }
    }
};

// Asynchronous line-by-line JSON parser using Boost.Asio stream
// Usage: construct with stream, call async_parse(cb, handler)
template<typename AsyncStream>
class AsyncJsonParser : public std::enable_shared_from_this<AsyncJsonParser<AsyncStream>> {
    AsyncStream& stream_;
    boost::asio::streambuf buf_;
    std::function<void(const nlohmann::json&)> cb_;
    std::string leftover_;
public:
    AsyncJsonParser(AsyncStream& stream) : stream_(stream) {}

    template<typename Handler>
    void async_parse(std::function<void(const nlohmann::json&)> cb, Handler handler) {
        cb_ = std::move(cb);
        do_read(std::move(handler));
    }
private:
    template<typename Handler>
    void do_read(Handler handler) {
        auto self = this->shared_from_this();
        boost::asio::async_read_until(stream_, buf_, '\n',
            [this, self, handler](boost::system::error_code ec, std::size_t n) mutable {
                if (ec) return handler(ec);
                std::istream is(&buf_);
                std::string line;
                std::getline(is, line);
                try {
                    auto obj = nlohmann::json::parse(line);
                    cb_(obj);
                } catch (...) {}
                do_read(std::move(handler));
            });
    }
};

} // namespace p2p
