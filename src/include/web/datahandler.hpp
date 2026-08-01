#pragma once

#include <boost/asio/ip/tcp.hpp>
#include <boost/asio.hpp>
#include <boost/beast.hpp>
#include <boost/beast/websocket/stream.hpp>
#include <cstdint>
#include <web/client.hpp>
#include <web/log.hpp>
#include <filesystem>
#include <iostream>
#include <string>
#include <vector>
// Custom definitions mapping network socket namespaces cleanly
namespace asio = ::boost::asio;
using tcp = asio::ip::tcp;
using ws_stream = boost::beast::websocket::stream<tcp::socket>;
using tcp = boost::asio::ip::tcp;
namespace websocket = boost::beast::websocket;
using WebSocketStreamPtr = std::shared_ptr<websocket::stream<tcp::socket>>;

namespace data {
enum class OP : int { NOP = 0, TX = 1, RX = 2, NEW = 4, CP = 8, MV = 16, DEL = 32, AP = 64};
enum class Result : int { SUCCESS = 0, FAILURE = 1, PENDING = 2 };
enum class FileResult : int {
  SUCCESS = 0,
  NOT_FOUND = 1,
  PERMISSION_DENIED = 2,
  READ_FAILED = 4,
  UNKNOWN_ERROR = 8,
  OPEN_FAILURE = 16,
  WRITE_FAILURE = 32,
  PENDING = 64
};

class WebSocket {
public:
    using error_code = boost::system::error_code;

    // The constructor handles automatic initialization down the chain
    WebSocket(p2p::web::ClientOptions _clientOpts = {"127.0.0.1", 4557}, int concurrency_hint = 2)
        : io(concurrency_hint),socket(io),resolv(io),ws(std::move(socket)),clientOpts(_clientOpts){}

    void connect() {
        auto res = resolv.resolve(clientOpts.host, std::to_string(clientOpts.port), ec);
        if (ec) {std::cerr << "Resolution failed: " << ec.message() << "\n";return;}

        asio::connect(ws.next_layer(), res.begin(), res.end(), ec);
        if (ec) {std::cerr << "Connection failed: " << ec.message() << "\n";return;}

        ws.handshake(clientOpts.host, "/", ec);
        if (ec) {std::cerr << "Handshake failed: " << ec.message() << "\n";return;}
    }
    boost::asio::io_context& IO(){return io;}

    ws_stream& WS(){return ws;}
    ws_stream& WsRef() {return this->ws;}
    template<typename T> void write(T writeData){ws.write(writeData);};

    template<typename T> void asyncWrite(T writeData){ws.async_write(boost::asio::buffer(writeData));};

    template<typename T> void read(T readData){ws.read(boost::asio::buffer(readData));};

    template<typename T> void asyncReac(T readData){ws.async_read(boost::asio::buffer(readData));};

// Variables are intentionally ordered by structural dependency constraints
private:
    boost::asio::io_context io;     // Context MUST be declared first
    tcp::socket socket;             // Socket depends on Context
    tcp::resolver resolv;           // Resolver depends on Context
public:
    ws_stream ws;                   // WebSocket stream wraps and owns the socket
    p2p::web::ClientOptions clientOpts;
    error_code ec;
};

struct DataProcessor{
    std::filesystem::path target;
    std::vector<uint8_t>* buffer;
    std::filesystem::path dest{""};
};

struct dataTransaction {
  OP cmd;
  ws_stream& ws;
  DataProcessor data;
  FileResult fileResult{FileResult::PENDING};
  Result status{Result::PENDING};
  };
} // namespace data


namespace data_ops {
using namespace data;
FileResult readBinaryFile           (dataTransaction& tx);
FileResult writeBinaryFile          (dataTransaction& tx);
FileResult appendBinaryFile         (dataTransaction& tx);
FileResult writeToFile              (dataTransaction& tx);
FileResult appendFile               (dataTransaction& tx);
FileResult moveFile                 (dataTransaction& tx);
FileResult sendBinaryToStdout       (dataTransaction& tx);
FileResult sendBinaryToWebSocket    (dataTransaction& tx);
FileResult readBinaryFromWebSocket  (dataTransaction& tx);
} // namespace data_ops
void handleDataSync(data::dataTransaction &tx);