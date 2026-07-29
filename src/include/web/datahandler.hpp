#pragma once

#include <boost/asio/ip/tcp.hpp>
#include <boost/asio.hpp>
#include <boost/beast.hpp>
#include <boost/beast/websocket/stream.hpp>
#include <cstdint>
#include<web/client.hpp>
#include <cstddef>
#include <filesystem>
#include <iostream>
#include <string>
#include <vector>
// Custom definitions mapping network socket namespaces cleanly
namespace asio = ::boost::asio;
using tcp = asio::ip::tcp;
using ws_stream = boost::beast::websocket::stream<tcp::socket>;

namespace data {
enum class OP : int { NOP = 0, TX = 1, RX = 2, NEW = 4, CP = 8, MV = 16, DEL = 32 };
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

struct dataTransaction {
  OP cmd;
  Result status{Result::PENDING};
  FileResult fileResult{FileResult::PENDING};
  std::filesystem::path targetPath;
  std::vector<std::string> filepaths; 
  std::filesystem::path outputPath{"/home/boopy"};
  std::vector<std::byte> memoryBuffer; 
  ws_stream* ws;  
  // Pass by value and std::move it into the member variable
  dataTransaction(std::string _targetPath, int reserveSize, std::string _outputPath, ws_stream* _ws, OP operation = OP::NOP)
  : targetPath(_targetPath), outputPath(_outputPath), ws(_ws){
    cmd=operation; filepaths.reserve(reserveSize); memoryBuffer.reserve(reserveSize);
  }};
} // namespace data

// Share state instances across different source files safely via extern declarations
extern std::vector<std::string> dataLogBuilder;
extern std::vector<std::string> activeServerFilePaths;
extern data::dataTransaction ThisDataTransaction;

namespace data_ops {
data::FileResult readBinaryFile(data::dataTransaction &tx);
data::FileResult writeBinaryFile(const data::dataTransaction &tx);
data::FileResult writeToFile(const data::dataTransaction &tx);
data::FileResult sendBinaryToStdout(const data::dataTransaction &tx);
data::FileResult sendBinaryToWebSocket(const data::dataTransaction tx);
data::FileResult readBinaryFromWebSocket(data::dataTransaction &tx); // Removed const so we can safely mutate memoryBuffer
} // namespace data_ops
void handleDataSync(data::dataTransaction &tx);
//void handleDataSync(data::dataTransaction &tx, ::ws_stream& ws);
