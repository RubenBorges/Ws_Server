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

} // namespace data
namespace asio = boost::asio;
using tcp = asio::ip::tcp;

class DataProcessor {
  std::vector<uint8_t> _buffer;
  std::vector<uint8_t> *_bufferList;
  std::vector<std::string> logBuilder;

public:
  using FileResult = data::FileResult;
  using ws_stream = boost::beast::websocket::stream<tcp::socket>;

  DataProcessor() : _buffer(), _bufferList(nullptr) {
    logBuilder.push_back(std::format(
        "T:{:%Y-%m-%d %H:%M:%S} -- NOTIFY:Initializing DataProcessor.",
        std::chrono::system_clock::now()));
  } // FIXED: removed extra semicolon
  // READING DATA
  FileResult readBinaryFile(const std::string &filePath) {
    logBuilder.push_back(
        std::format("T:{:%Y-%m-%d %H:%M:%S} -- NOTIFY:Reading File [{}].",
                    std::chrono::system_clock::now(), filePath));

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
    logBuilder.push_back(
        std::format("T:{:%Y-%m-%d %H:%M:%S} -- NOTIFY:Reading File [{}].",
                    std::chrono::system_clock::now(), filePath));

    if (!std::filesystem::is_regular_file(filePath)) {
      std::cerr << "Path is not a valid file: " << filePath << "\n";
      logBuilder.push_back(std::format(
          "T:{:%Y-%m-%d %H:%M:%S} -- ERROR:Path is not a valid File [{}].",
          std::chrono::system_clock::now(), filePath));
      return FileResult::NOT_FOUND; // FIXED: Avoid returning empty braces '{}'
                                    // for explicit enums
    }

    std::ifstream file(filePath, std::ios::binary | std::ios::ate);
    if (!file.is_open()) {
      std::cerr << "Failed to open file: " << filePath << "\n";
      logBuilder.push_back(std::format(
          "T:{:%Y-%m-%d %H:%M:%S} -- ERROR:Failed to open File [{}].",
          std::chrono::system_clock::now(), filePath));
      return FileResult::OPEN_FAILURE;
    }
    std::streamsize size = file.tellg();
    file.seekg(0, std::ios::beg);
    std::vector<uint8_t> buffer(size);

    if (file.read(reinterpret_cast<char *>(buffer.data()), size)) {
      _buffer = buffer;
      logBuilder.push_back(std::format(
          "T:{:%Y-%m-%d %H:%M:%S} -- NOTIFY:File Successfully Read [{}].",
          std::chrono::system_clock::now(), filePath));
      return FileResult::SUCCESS;
    }
    logBuilder.push_back(
        std::format("T:{:%Y-%m-%d %H:%M:%S} -- ERROR:Read Failure [{}].",
                    std::chrono::system_clock::now(), filePath));
    return FileResult::READ_FAILED;
  }
  // SENDING DATA
  void sendBinaryData(const uint8_t *data, size_t size) {
    logBuilder.push_back(
        std::format("T:{:%Y-%m-%d %H:%M:%S} -- NOTIFY:Writing data to stdout",
                    std::chrono::system_clock::now()));
    std::cout << "Sending " << size << " bytes of binary data...\n";

    for (size_t i = 0; i < size; ++i) {
      std::cout << "0x" << std::hex << static_cast<int>(data[i]) << " ";
    }
    std::cout << "\n";
    logBuilder.push_back(std::format(
        "T:{:%Y-%m-%d %H:%M:%S} -- NOTIFY:Data successfully written to stdout.",
        std::chrono::system_clock::now()));
  }

  void sendBinaryData(const uint8_t *data, size_t size, ws_stream &ws) {
    logBuilder.push_back(std::format(
        "T:{:%Y-%m-%d %H:%M:%S} -- NOTIFY:Writing data to WebSocket",
        std::chrono::system_clock::now()));
    std::cout << "Sending " << size
              << " bytes of binary data via WebSocket...\n";
    ws.binary(true);
    boost::beast::error_code ec;
    ws.write(boost::asio::buffer(data, size), ec);
    if (ec) {
      std::cerr << "WebSocket send failed: " << ec.message() << "\n";
      logBuilder.push_back(std::format(
          "T:{:%Y-%m-%d %H:%M:%S} -- ERROR:Failed writing data to WebSocket",
          std::chrono::system_clock::now()));
    } else {
      logBuilder.push_back(std::format("T:{:%Y-%m-%d %H:%M:%S} -- NOTIFY:Data successfully written to WebSocket",
                                       std::chrono::system_clock::now()));
    }
  }
  // WRITE DATA TO FILE
  FileResult writeBinaryFile(const std::string &path,
                             const std::vector<uint8_t> &buffer) {
  logBuilder.push_back(std::format("T:{:%Y-%m-%d %H:%M:%S} -- NOTIFY:Writing data to File: {}",std::chrono::system_clock::now(),path));
    std::ofstream outFile(path, std::ios::binary);
    if (!outFile.is_open()){
    logBuilder.push_back(std::format("T:{:%Y-%m-%d %H:%M:%S} -- ERROR:Failed to open File: {}",std::chrono::system_clock::now(),path));
    return FileResult::OPEN_FAILURE;
    }

    std::span<const uint8_t> dataSpan(buffer);
    auto byteSpan = std::as_bytes(dataSpan);
    outFile.write(reinterpret_cast<const char *>(byteSpan.data()),
                  byteSpan.size_bytes());
    logBuilder.push_back(std::format("T:{:%Y-%m-%d %H:%M:%S} -- NOTIFY:Data successfully written to File: {}",std::chrono::system_clock::now(),path));
    return FileResult::SUCCESS;
  }

  FileResult writeBinaryFile(const std::filesystem::path &path,
                             const std::vector<uint8_t> &buffer) {
    logBuilder.push_back(std::format("T:{:%Y-%m-%d %H:%M:%S} -- NOTIFY:Writing data to File: {}",std::chrono::system_clock::now(),path));
if (path.has_parent_path()) {
      std::filesystem::create_directories(path.parent_path());
    }
    std::ofstream outFile(path, std::ios::binary);
    if (!outFile.is_open()){
    logBuilder.push_back(std::format("T:{:%Y-%m-%d %H:%M:%S} -- ERROR:Failed writing data to File: {}",std::chrono::system_clock::now(),path));
    return FileResult::OPEN_FAILURE;
    }

    std::span<const uint8_t> dataSpan(buffer);
    auto byteSpan = std::as_bytes(dataSpan);
    outFile.write(reinterpret_cast<const char *>(byteSpan.data()),
                  byteSpan.size_bytes());
    logBuilder.push_back(std::format("T:{:%Y-%m-%d %H:%M:%S} -- NOTIFY:Data successfully written to File: {}",std::chrono::system_clock::now(),path));
    return FileResult::SUCCESS;
  }
};