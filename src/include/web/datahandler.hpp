#pragma once

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

// Unified state struct matching the 'loginRequest' layout pattern
struct dataTransaction {
  OP cmd{OP::NOP};
  Result status{Result::PENDING};
  std::filesystem::path targetPath;
  std::vector<uint8_t> memoryBuffer; // Scopes the raw byte context straight to the request
};

} // namespace data

namespace asio = boost::asio;
using tcp = asio::ip::tcp;
using ws_stream = boost::beast::websocket::stream<tcp::socket>;

// Global logging and state tables matching your activeSession / logBuilder style
inline std::vector<std::string> dataLogBuilder;
inline std::vector<std::string> activeServerFilePaths;
inline data::dataTransaction ThisDataTransaction;

namespace data_ops {

// READING DATA
inline data::FileResult readBinaryFile(data::dataTransaction &tx) {
  dataLogBuilder.push_back(
      std::format("T:{:%Y-%m-%d %H:%M:%S} -- NOTIFY:Reading File [{}].",
                  std::chrono::system_clock::now(), tx.targetPath.string()));

  if (!std::filesystem::is_regular_file(tx.targetPath)) {
    std::cerr << "Path is not a valid file: " << tx.targetPath << "\n";
    dataLogBuilder.push_back(std::format(
        "T:{:%Y-%m-%d %H:%M:%S} -- ERROR:Path is not a valid File [{}].",
        std::chrono::system_clock::now(), tx.targetPath.string()));
    return data::FileResult::NOT_FOUND;
  }

  std::ifstream file(tx.targetPath, std::ios::binary | std::ios::ate);
  if (!file.is_open()) {
    std::cerr << "Failed to open file: " << tx.targetPath << "\n";
    dataLogBuilder.push_back(std::format(
        "T:{:%Y-%m-%d %H:%M:%S} -- ERROR:Failed to open File [{}].",
        std::chrono::system_clock::now(), tx.targetPath.string()));
    return data::FileResult::OPEN_FAILURE;
  }

  std::streamsize size = file.tellg();
  file.seekg(0, std::ios::beg);
  
  tx.memoryBuffer.resize(size);
  if (file.read(reinterpret_cast<char *>(tx.memoryBuffer.data()), size)) {
    dataLogBuilder.push_back(std::format(
        "T:{:%Y-%m-%d %H:%M:%S} -- NOTIFY:File Successfully Read [{}].",
        std::chrono::system_clock::now(), tx.targetPath.string()));
    return data::FileResult::SUCCESS;
  }

  dataLogBuilder.push_back(
      std::format("T:{:%Y-%m-%d %H:%M:%S} -- ERROR:Read Failure [{}].",
                  std::chrono::system_clock::now(), tx.targetPath.string()));
  return data::FileResult::READ_FAILED;
}

// WRITE DATA TO FILE
inline data::FileResult writeBinaryFile(const data::dataTransaction &tx) {
  dataLogBuilder.push_back(std::format("T:{:%Y-%m-%d %H:%M:%S} -- NOTIFY:Writing data to File: {}",
                                       std::chrono::system_clock::now(), tx.targetPath.string()));
  
  if (tx.targetPath.has_parent_path()) {
    std::filesystem::create_directories(tx.targetPath.parent_path());
  }

  std::ofstream outFile(tx.targetPath, std::ios::binary);
  if (!outFile.is_open()) {
    dataLogBuilder.push_back(std::format("T:{:%Y-%m-%d %H:%M:%S} -- ERROR:Failed writing data to File: {}",
                                         std::chrono::system_clock::now(), tx.targetPath.string()));
    return data::FileResult::OPEN_FAILURE;
  }

  std::span<const uint8_t> dataSpan(tx.memoryBuffer);
  auto byteSpan = std::as_bytes(dataSpan);
  outFile.write(reinterpret_cast<const char *>(byteSpan.data()), byteSpan.size_bytes());
  
  dataLogBuilder.push_back(std::format("T:{:%Y-%m-%d %H:%M:%S} -- NOTIFY:Data successfully written to File: {}",
                                       std::chrono::system_clock::now(), tx.targetPath.string()));
  return outFile.good() ? data::FileResult::SUCCESS : data::FileResult::READ_FAILED;
}

// SENDING DATA TO STDOUT
inline void sendBinaryToStdout(const data::dataTransaction &tx) {
  dataLogBuilder.push_back(
      std::format("T:{:%Y-%m-%d %H:%M:%S} -- NOTIFY:Writing data to stdout",
                  std::chrono::system_clock::now()));
  std::cout << "Sending " << tx.memoryBuffer.size() << " bytes of binary data...\n";

  for (size_t i = 0; i < tx.memoryBuffer.size(); ++i) {
    std::cout << "0x" << std::hex << static_cast<int>(tx.memoryBuffer[i]) << " ";
  }
  std::cout << "\n";
  dataLogBuilder.push_back(std::format(
      "T:{:%Y-%m-%d %H:%M:%S} -- NOTIFY:Data successfully written to stdout.",
      std::chrono::system_clock::now()));
}

// SENDING DATA TO WEBSOCKET
inline void sendBinaryToWebSocket(ws_stream &ws, const data::dataTransaction &tx) {
  dataLogBuilder.push_back(std::format(
      "T:{:%Y-%m-%d %H:%M:%S} -- NOTIFY:Writing data to WebSocket",
      std::chrono::system_clock::now()));
  std::cout << "Sending " << tx.memoryBuffer.size() << " bytes of binary data via WebSocket...\n";
  
  ws.binary(true);
  boost::beast::error_code ec;
  ws.write(boost::asio::buffer(tx.memoryBuffer.data(), tx.memoryBuffer.size()), ec);
  
  if (ec) {
    std::cerr << "WebSocket send failed: " << ec.message() << "\n";
    dataLogBuilder.push_back(std::format(
        "T:{:%Y-%m-%d %H:%M:%S} -- ERROR:Failed writing data to WebSocket",
        std::chrono::system_clock::now()));
  } else {
    dataLogBuilder.push_back(std::format("T:{:%Y-%m-%d %H:%M:%S} -- NOTIFY:Data successfully written to WebSocket",
                                         std::chrono::system_clock::now()));
  }
}

} // namespace data_ops

// Explicit central execution dispatcher mapping exactly to the login() layout
inline void handleDataSync(data::dataTransaction &tx) {
  switch (tx.cmd) {
    case data::OP::RX: { // Read operation execution path
      data::FileResult res = data_ops::readBinaryFile(tx);
      if (res == data::FileResult::SUCCESS) {
        tx.status = data::Result::SUCCESS;
      } else {
        tx.status = data::Result::FAILURE;
      }
      break;
    }

    case data::OP::TX: { // Write operation execution path
      data::FileResult res = data_ops::writeBinaryFile(tx);
      if (res == data::FileResult::SUCCESS) {
        tx.status = data::Result::SUCCESS;
      } else {
        tx.status = data::Result::FAILURE;
      }
      break;
    }

    case data::OP::NEW: { // Directory listing initialization execution path
      dataLogBuilder.push_back(std::format(
          "T:{:%Y-%m-%d %H:%M:%S} -- NOTIFY:Crawl processing initiated.",
          std::chrono::system_clock::now()));
      
      DirectoryCrawler dr(tx.targetPath.string());
      dr.crawlRecursively(activeServerFilePaths);
      tx.status = data::Result::SUCCESS;
      break;
    }

    case data::OP::DEL: {
      dataLogBuilder.push_back(std::format(
          "T:{:%Y-%m-%d %H:%M:%S} -- NOTIFY:Delete processing initiated for target: {}",
          std::chrono::system_clock::now(), tx.targetPath.string()));
          
      boost::system::error_code ec;
      std::filesystem::remove_all(tx.targetPath, ec);
      if (!ec) {
        tx.status = data::Result::SUCCESS;
      } else {
        tx.status = data::Result::FAILURE;
      }
      break;
    }

    case data::OP::NOP:
    default:
      tx.status = data::Result::FAILURE;
      break;
  }
}
