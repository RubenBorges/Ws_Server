#include "dir_crawler.hpp"
#include <BPY/util.hpp>
#include <algorithm>
#include <boost/asio/buffer.hpp>
#include <boost/asio/ip/tcp.hpp> 
#include <boost/beast/websocket.hpp>
#include <boost/random/random_device.hpp>
#include <boost/uuid/uuid.hpp>
#include <boost/uuid/uuid_generators.hpp>
#include <boost/uuid/uuid_io.hpp>
#include <chrono>
#include <cstddef>
#include <cstdint>
#include <cstdlib>
#include <filesystem>
#include <format>
#include <fstream>
#include <iostream>
#include <jwt-cpp/jwt.h>
#include <print>
#include <span>
#include <string>
#include <vector>

namespace data {
enum class OP : int { NOP = 0, TX = 1, RX = 2, NEW = 4, DEL = 8 };
enum class Result : int { SUCCESS = 0, FAILURE = 1, PENDING = 2 };
enum class FileResult : int {
  SUCCESS = 0,
  NOT_FOUND = 1,         
  PERMISSION_DENIED = 2, 
  READ_FAILED = 4,       
  UNKNOWN_ERROR = 8,     
  OPEN_FAILURE = 16
};
struct dataRequest {
  std::vector<std::string> &filepaths;
  OP op{OP::NOP}; 
  Result result{Result::PENDING}; 
};
} // namespace data (FIXED: removed redundant semicolon)

// FIXED: Namespace alias moved outside of the class definition
namespace asio = boost::asio;
using tcp = asio::ip::tcp;

class DataProcessor {
  using FileResult = data::FileResult;
  using wsstream = boost::beast::websocket::stream<boost::asio::ip::tcp::socket>;
  
  std::vector<uint8_t> _buffer;
  std::vector<uint8_t> *_bufferList;

public:
  // FIXED: Using class-scoped type mappings instead of namespace aliases
  using ws_stream = boost::beast::websocket::stream<tcp::socket>;

  DataProcessor() : _buffer(), _bufferList(nullptr) {} // FIXED: removed extra semicolon

  FileResult readBinaryFile(const std::string &filePath) {
    std::ifstream file(filePath, std::ios::binary | std::ios::ate);
    if (!file.is_open()) {
      std::cerr << "Failed to open file: " << filePath << "\n";
      return FileResult::OPEN_FAILURE;
    }
    std::streamsize size = file.tellg();
    file.seekg(0, std::ios::beg);
    std::vector<uint8_t> buffer(size);
    if (file.read(reinterpret_cast<char *>(buffer.data()), size)) {
      _buffer = buffer;
      return FileResult::SUCCESS;
    }
    return FileResult::READ_FAILED;
  }

  FileResult readBinaryFile(const std::filesystem::path &filePath) {
    if (!std::filesystem::is_regular_file(filePath)) {
      std::cerr << "Path is not a valid file: " << filePath << "\n";
      return FileResult::NOT_FOUND; // FIXED: Avoid returning empty braces '{}' for explicit enums
    }

    std::ifstream file(filePath, std::ios::binary | std::ios::ate);
    if (!file.is_open()) {
      std::cerr << "Failed to open file: " << filePath << "\n";
      return FileResult::OPEN_FAILURE;
    }
    std::streamsize size = file.tellg();
    file.seekg(0, std::ios::beg);
    std::vector<uint8_t> buffer(size);

    if (file.read(reinterpret_cast<char *>(buffer.data()), size)) {
      _buffer = buffer;
      return FileResult::SUCCESS;
    }
    return FileResult::READ_FAILED;
  } // FIXED: removed trailing semicolon

  void sendBinaryData(const uint8_t *data, size_t size) {
    std::cout << "Sending " << size << " bytes of binary data...\n";

    for (size_t i = 0; i < std::min(size, size_t(5)); ++i) {
      std::cout << "0x" << std::hex << static_cast<int>(data[i]) << " ";
    }
    std::cout << "\n";
  } // FIXED: removed trailing semicolon

  FileResult writeBinaryFile(const std::string &path, const std::vector<uint8_t> &buffer) {
    std::ofstream outFile(path, std::ios::binary);
    if (!outFile.is_open())
      return FileResult::OPEN_FAILURE;

    std::span<const uint8_t> dataSpan(buffer);
    auto byteSpan = std::as_bytes(dataSpan);
    outFile.write(reinterpret_cast<const char *>(byteSpan.data()), byteSpan.size_bytes());

    return FileResult::SUCCESS;
  } // FIXED: removed trailing semicolon

  FileResult writeBinaryFile(const std::filesystem::path &path, const std::vector<uint8_t> &buffer) {
    if (path.has_parent_path()) {
      std::filesystem::create_directories(path.parent_path());
    }

    std::ofstream outFile(path, std::ios::binary);
    if (!outFile.is_open())
      return FileResult::OPEN_FAILURE;

    std::span<const uint8_t> dataSpan(buffer);
    auto byteSpan = std::as_bytes(dataSpan);
    outFile.write(reinterpret_cast<const char *>(byteSpan.data()), byteSpan.size_bytes());
    return FileResult::SUCCESS;
  } // FIXED: removed trailing semicolon

  void sendBinaryData(ws_stream &ws, const uint8_t *data, size_t size) {
    std::cout << "Sending " << size << " bytes of binary data via WebSocket...\n";
    for (size_t i = 0; i < std::min(size, size_t(5)); ++i) {
      std::cout << "0x" << std::hex << static_cast<int>(data[i]) << " ";
    }
    std::cout << "\n";

    ws.binary(true);
    boost::beast::error_code ec;
    ws.write(boost::asio::buffer(data, size), ec);

    if (ec) {
      std::cerr << "WebSocket send failed: " << ec.message() << "\n";
    }
  } // FIXED: removed trailing semicolon
}; // FIXED: This closing brace successfully seals the DataProcessor class
