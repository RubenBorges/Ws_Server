#pragma once

#include <boost/asio/ip/tcp.hpp>
#include <boost/beast/websocket/stream.hpp>
#include <cstdint>
#include <filesystem>
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

// Upgrade dataTransaction to handle BOTH binary data and multi-path queries
struct dataTransaction {
  OP cmd{OP::NOP};
  Result status{Result::PENDING};
  FileResult fileResult{FileResult::PENDING};
  
  // 1. Single-file target path (used for RX, TX, DEL)
  std::filesystem::path targetPath;
  
  // 2. Collection path pool (Replaces dataRequest::filepaths!)
  // Useful for directory crawling (NEW), multi-file copying (CP), or moving (MV)
  std::vector<std::string> filepaths; 
  
  // 3. Holds the absolute root folder destination for operations
  std::filesystem::path outputPath{"/home/boopy"};
  
  // 4. In-memory data payload block
  std::vector<uint8_t> memoryBuffer; 
  
  // 5. Active non-copyable client connection channel
  ws_stream* socketContext{nullptr}; 
};
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
data::FileResult sendBinaryToWebSocket(const data::dataTransaction &tx);
data::FileResult readBinaryFromWebSocket(data::dataTransaction &tx); // Removed const so we can safely mutate memoryBuffer
} // namespace data_ops
void handleDataSync(data::dataTransaction &tx);
void handleDataSync(data::dataTransaction &tx, ::ws_stream& ws);
