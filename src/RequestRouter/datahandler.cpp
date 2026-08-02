#include <RequestRouter/loginhandler.hpp>
#include <RequestRouter/datahandler.hpp>
#include <BPY/util.hpp>
#include <log.hpp>
#include <boost/beast.hpp>
#include <boost/asio.hpp>
#include <boost/beast/core.hpp>
#include <boost/beast/websocket.hpp>
#include <boost/asio/connect.hpp>
#include <boost/asio/ip/tcp.hpp>
#include <boost/beast/core/stream_traits.hpp>
#include <boost/asio/streambuf.hpp>
#include <filesystem>
#include <iosfwd>
#include <system_error>
#include <fstream>
#include <iostream>
#include <variant>
#include <format>
#include <chrono>
#include <print>
#include <cstddef>

using FileResult = data::FileResult;
using dataTransaction = data::dataTransaction;
using RequestVariant = std::variant<loginRequest, dataTransaction>;
namespace asio = boost::asio;
using tcp = asio::ip::tcp;
using ws_stream = boost::beast::websocket::stream<tcp::socket>;

RequestVariant req;
namespace data_ops {
FileResult readBinaryFile(dataTransaction &tx) {
    const auto &filePath = tx.data.target;

    // Validate File Existence
    if (!std::filesystem::is_regular_file(filePath)) {
        std::println(std::cerr, "Path is not a valid file: {}", filePath);
        logBuilder.push_back(std::format("T:{:%Y-%m-%d %H:%M:%S} -- ERROR:Path is not a valid File [{}].",  std::chrono::system_clock::now(), filePath));
        return FileResult::NOT_FOUND; 
    }

    // Fetch file size reliably using OS Metadata (Bypasses stream open errors)
    std::error_code ec;
    std::uintmax_t exactSize = std::filesystem::file_size(filePath, ec);
    tx.data.buffer->resize(exactSize); // Clear buffer before reading
    if (ec) {
        std::println(std::cerr, "OS blocked reading file size metadata: {}", ec.message());
        logBuilder.push_back(std::format("T:{:%Y-%m-%d %H:%M:%S} -- ERROR:Metadata Read Failure [{}].",  std::chrono::system_clock::now(), filePath));
        return FileResult::OPEN_FAILURE; // OS level block (Permissions/Sharing violation)
    }

    if (exactSize == 0) {
        tx.data.buffer->clear();
        std::println("READING FILE: {} [0 bytes]", filePath);
        logBuilder.push_back(std::format("T:{:%Y-%m-%d %H:%M:%S} -- NOTIFY:File Successfully Read (Empty) [{}].", std::chrono::system_clock::now(), filePath));
        return FileResult::SUCCESS;
    }

    // Open File Stream normally
    std::ifstream file(filePath, std::ios::binary);
    if (!file.is_open()) {
        std::println(std::cerr, "Failed to open file stream: {}", filePath);
        logBuilder.push_back(std::format("T:{:%Y-%m-%d %H:%M:%S} -- ERROR:Failed to open File Stream [{}].", std::chrono::system_clock::now(), filePath));
        return FileResult::OPEN_FAILURE;
    }

    // Perform allocations safely
    tx.data.buffer->resize(static_cast<std::size_t>(exactSize));
    std::println("READING FILE: {} [{} bytes]", filePath, exactSize);

    if (file.read(reinterpret_cast<char *>(tx.data.buffer->data()), static_cast<std::streamsize>(exactSize))) {
        for (auto byte : *tx.data.buffer) {
            std::print("{:02X} ", static_cast<uint8_t>(byte));
        }
        std::println(""); 
        logBuilder.push_back(std::format("T:{:%Y-%m-%d %H:%M:%S} -- NOTIFY:File Successfully Read [{}].", std::chrono::system_clock::now(), filePath));
        return FileResult::SUCCESS;
    }

    logBuilder.push_back(std::format("T:{:%Y-%m-%d %H:%M:%S} -- ERROR:Read Failure [{}].", std::chrono::system_clock::now(), filePath));
    return FileResult::READ_FAILED;
}

FileResult writeBinaryFile(dataTransaction &tx) {
    if (tx.data.target.has_parent_path()) std::filesystem::create_directories(tx.data.target.parent_path());
    std::ofstream outputFile(tx.data.target, std::ios::out|std::ios::binary);

    if (!outputFile.is_open()){
      logBuilder.push_back(std::format("T:{:%Y-%m-%d %H:%M:%S} -- ERROR:Failed to open File Stream [{}].", std::chrono::system_clock::now(), tx.data.target));
      return FileResult::OPEN_FAILURE;
    }

    outputFile.write(reinterpret_cast<const char*>(tx.data.buffer->data()), tx.data.buffer->size());
    return outputFile.good() ? FileResult::SUCCESS : FileResult::WRITE_FAILURE;
}
FileResult appendBinaryFile(dataTransaction &tx) {
    if (tx.data.target.has_parent_path()) std::filesystem::create_directories(tx.data.target.parent_path());
    std::ofstream outputFile(tx.data.target, std::ios::out|std::ios::binary|std::ios::app);

    if (!outputFile.is_open()){
      logBuilder.push_back(std::format("T:{:%Y-%m-%d %H:%M:%S} -- ERROR:Failed to open File Stream [{}].", std::chrono::system_clock::now(), tx.data.target));
      return FileResult::OPEN_FAILURE;
    }

    outputFile.write(reinterpret_cast<const char*>(tx.data.buffer->data()), tx.data.buffer->size());
    return outputFile.good() ? FileResult::SUCCESS : FileResult::WRITE_FAILURE;
}

FileResult writeToFile(dataTransaction &tx) {
    if (tx.data.target.has_parent_path()) std::filesystem::create_directories(tx.data.target.parent_path());
    std::ofstream outputFile(tx.data.target, std::ios::out);
    
    if (!outputFile.is_open()){
      logBuilder.push_back(std::format("T:{:%Y-%m-%d %H:%M:%S} -- ERROR:Failed to open File Stream [{}].", std::chrono::system_clock::now(), tx.data.target));
      return FileResult::OPEN_FAILURE;
    }

    outputFile.write(reinterpret_cast<const char*>(tx.data.buffer->data()), tx.data.buffer->size());
    return outputFile.good() ? FileResult::SUCCESS : FileResult::WRITE_FAILURE;
}

FileResult moveFile(dataTransaction &tx){
  auto res = readBinaryFile(tx);

  if (res == FileResult::SUCCESS){
    if (tx.data.target.has_parent_path()) std::filesystem::create_directories(tx.data.target.parent_path());
    std::ofstream destFile(tx.data.dest, std::ios::out|std::ios::binary);

    if (!destFile.is_open()){
      logBuilder.push_back(std::format("T:{:%Y-%m-%d %H:%M:%S} -- ERROR:Failed to open File Stream [{}].", std::chrono::system_clock::now(), tx.data.dest));
      return FileResult::OPEN_FAILURE;
    }

    destFile.write(reinterpret_cast<const char*>(tx.data.buffer->data()), tx.data.buffer->size());
    res = destFile.good() ? FileResult::SUCCESS : FileResult::WRITE_FAILURE;
    std::filesystem::remove_all(tx.data.target);
  }

  return res;
}

FileResult appendFile(dataTransaction &tx) {
    if (tx.data.target.has_parent_path()) std::filesystem::create_directories(tx.data.target.parent_path());
    std::ofstream outFile(tx.data.target, std::ios::out|std::ios::app);

    if (!outFile.is_open()){
      logBuilder.push_back(std::format("T:{:%Y-%m-%d %H:%M:%S} -- ERROR:Failed to open File Stream [{}].", std::chrono::system_clock::now(), tx.data.target));
      return FileResult::OPEN_FAILURE;
    }

    outFile.write(reinterpret_cast<const char*>(tx.data.buffer->data()), tx.data.buffer->size());

    return outFile.good() ? FileResult::SUCCESS : FileResult::WRITE_FAILURE;
}

FileResult sendBinaryToStdout(dataTransaction &tx) {
    std::println("Streaming {} bytes directly to standard output:", tx.data.buffer->size());
    logBuilder.push_back(std::format("T:{:%Y-%m-%d %H:%M:%S} -- NOTIFY:Streaming {} bytes directly to standard output:", std::chrono::system_clock::now(), tx.data.buffer->size()));

    for (auto byte: *tx.data.buffer) std::print("{:02X} ", byte);
    std::cout << "\n";

    return FileResult::SUCCESS;
}

FileResult sendBinaryToWebSocket(dataTransaction& tx) {
    if (!tx.ws.is_open()) {
      logBuilder.push_back(std::format("T:{:%Y-%m-%d %H:%M:%S} -- ERROR:WebSocket is not open.", std::chrono::system_clock::now()));
      return FileResult::WRITE_FAILURE;
    }

    boost::beast::error_code ec;
    tx.ws.binary(true);
    tx.ws.write(boost::asio::buffer(tx.data.buffer->data(), tx.data.buffer->size()));
    
    return ec ? FileResult::WRITE_FAILURE : FileResult::SUCCESS;
}

FileResult readBinaryFromWebSocket(dataTransaction &tx) {
    if (!tx.ws.is_open()) {
      logBuilder.push_back(std::format("T:{:%Y-%m-%d %H:%M:%S} -- ERROR:WebSocket is not open.", std::chrono::system_clock::now()));
      return FileResult::READ_FAILED;
    }

    boost::beast::flat_buffer dynamic_buffer;
    boost::beast::error_code ec;
    tx.ws.binary(true);
    tx.ws.read(dynamic_buffer);
    if (ec) {
      logBuilder.push_back(std::format("T:{:%Y-%m-%d %H:%M:%S} -- ERROR:Failed to read from WebSocket.", std::chrono::system_clock::now()));
      return FileResult::READ_FAILED;
    }
    size_t bytes_received = dynamic_buffer.size();
    tx.data.buffer->resize(bytes_received); 
    boost::asio::buffer_copy(boost::asio::buffer(tx.data.buffer->data(), tx.data.buffer->size()), dynamic_buffer.data());
    
    return FileResult::SUCCESS;
}
} // namespace data_ops

void handleDataSync(dataTransaction &tx){
  using namespace data;
  using namespace data_ops;
  std::error_code ec;

  switch (tx.cmd) {
    case OP::RX: { //Receive
      logBuilder.push_back(std::format("T:{:%Y-%m-%d %H:%M:%S} -- NOTIFY:Receiving file transfer initiated for target: {}",std::chrono::system_clock::now(), tx.data.target));
      tx.fileResult = readBinaryFromWebSocket(tx);
      if (tx.fileResult == FileResult::SUCCESS) {
        tx.fileResult = writeBinaryFile(tx);
        tx.status = (tx.fileResult== FileResult::SUCCESS) ? data::Result::SUCCESS : data::Result::FAILURE;
      } else {tx.status = data::Result::FAILURE;}
      break;
    }

  case OP::TX: //SEND
    logBuilder.push_back(std::format("T:{:%Y-%m-%d %H:%M:%S} -- NOTIFY:Transfer initiated for target: {}",std::chrono::system_clock::now(), tx.data.target));
    tx.fileResult = readBinaryFile(tx);
    if (tx.fileResult == FileResult::SUCCESS) tx.fileResult = sendBinaryToWebSocket(tx);
    tx.status = ( tx.fileResult == FileResult::SUCCESS) ? Result::SUCCESS: Result::FAILURE;
    break;

  case OP::NEW: {  //NEW FILE CREATION
    logBuilder.push_back(std::format("T:{:%Y-%m-%d %H:%M:%S} -- NOTIFY:New File Creation processing initiated {}",std::chrono::system_clock::now(),tx.data.target));
    tx.fileResult= (bpy::utility::createEmptyFile(tx.data.target))? FileResult::SUCCESS : FileResult::OPEN_FAILURE;
    tx.status = (tx.fileResult == FileResult::SUCCESS)? Result::SUCCESS : Result::FAILURE;
    break;
  }

  case OP::DEL: //DELETION
    logBuilder.push_back(std::format("T:{:%Y-%m-%d %H:%M:%S} -- NOTIFY:Delete processing initiated for target: {}",std::chrono::system_clock::now(), tx.data.target));
    std::filesystem::remove_all(tx.data.target, ec);
    tx.status = (!ec) ? Result::SUCCESS : Result::FAILURE;
    tx.fileResult = (tx.status == Result::SUCCESS)? FileResult::SUCCESS : FileResult::UNKNOWN_ERROR;
    break;

  case OP::CP: //COPY
    logBuilder.push_back(std::format("T:{:%Y-%m-%d %H:%M:%S} -- NOTIFY:Copying initiated for target: {}",std::chrono::system_clock::now(), tx.data.target));
    tx.fileResult = readBinaryFile(tx);
    std::cout << "BUFFER SIZE: " << tx.data.buffer->size() << std::endl;
    if (tx.fileResult == FileResult::SUCCESS) {
      tx.data.target = tx.data.dest; // Update target to destination for writing
      tx.fileResult = data_ops::writeBinaryFile(tx);
    }
    tx.status = ( tx.fileResult == FileResult::SUCCESS) ? Result::SUCCESS : Result::FAILURE;
    break;

  case OP::MV: //FILE MOVE
    logBuilder.push_back(std::format("T:{:%Y-%m-%d %H:%M:%S} -- NOTIFY:Move initiated for target: {}",std::chrono::system_clock::now(), tx.data.target));
    tx.fileResult = moveFile(tx);
    tx.status = (tx.fileResult==FileResult::SUCCESS) ? Result::SUCCESS : Result::FAILURE;
    break;
  
  case OP::AP: //APPEND
    logBuilder.push_back(std::format("T:{:%Y-%m-%d %H:%M:%S} -- NOTIFY:Appending to target: {}",std::chrono::system_clock::now(), tx.data.target));
    tx.fileResult = appendFile(tx);
    break;
    
  case OP::NOP: //NO OPERATION; IGNORE REQUEST
    logBuilder.push_back(std::format("T:{:%Y-%m-%d %H:%M:%S} -- NOTIFY:No OP Selected. Ignoring Transaction Request",std::chrono::system_clock::now()));
    tx.fileResult = FileResult::SUCCESS;
    tx.status = Result::SUCCESS;
    break;
  }
}
