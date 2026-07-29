#pragma once

#include <BPY/util.hpp>
#include <boost/asio/buffer.hpp>
#include <boost/asio/ip/tcp.hpp>
#include <boost/beast/websocket.hpp>
#include <cstdint>
#include <filesystem>
#include <string>
#include <vector>
// Mapping network socket namespaces
namespace asio = boost::asio;
using tcp = asio::ip::tcp;
using ws_stream = boost::beast::websocket::stream<tcp::socket>;
// Forward declaration of the stream type to keep this header light
namespace boost::beast::websocket {
    template <class NextLayer, bool deflateSupported> class stream;
}
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
  WRITE_FAILURE =32,
  PENDING = 64
};

struct dataRequest {
  std::vector<std::string> &filepaths;
  OP op{OP::NOP};
  Result result{Result::PENDING};
};

struct dataTransaction {
  
  OP cmd{OP::NOP};
  Result status{Result::PENDING};
  FileResult fileResult{FileResult::PENDING};
  std::filesystem::path targetPath;
  std::vector<uint8_t> memoryBuffer; 
  ws_stream* socketContext{nullptr}; 
};
} // namespace data


// Share state instances across different code files safely via extern declarations
extern std::vector<std::string> dataLogBuilder;
extern std::vector<std::string> activeServerFilePaths;
extern data::dataTransaction ThisDataTransaction;

namespace data_ops {
data::FileResult readBinaryFile(data::dataTransaction &tx);
data::FileResult writeBinaryFile(const data::dataTransaction &tx);
data::FileResult writeToFile(const data::dataTransaction &tx );
void sendBinaryToStdout(const data::dataTransaction &tx);
void sendBinaryToWebSocket(ws_stream &ws, const data::dataTransaction &tx);
} // namespace data_ops

void handleDataSync(data::dataTransaction &tx);
