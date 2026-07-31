#include <BPY/util.hpp>
#include <boost/beast.hpp>
#include <boost/asio.hpp>
#include <boost/beast/core.hpp>
#include <boost/beast/websocket.hpp>
#include <boost/asio/connect.hpp>
#include <boost/asio/ip/tcp.hpp>
#include <boost/beast/core/stream_traits.hpp>
#include <boost/asio/streambuf.hpp>
#include <filesystem>
#include <system_error>
#include <web/datahandler.hpp>
#include <web/router.hpp>
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
FileResult readBinaryFile(dataTransaction &tx, DataProcessor& data) {
    if (!std::filesystem::is_regular_file(data.target)) return FileResult::NOT_FOUND;
    std::ifstream file(data.target, std::ios::in|std::ios::binary);
    if (!file.is_open()) return FileResult::OPEN_FAILURE;
    std::streamsize size = file.tellg();
    file.seekg(0, std::ios::beg);
    data.buffer.reserve(size);
    std::println("READING FILE");
    return ( file.read(reinterpret_cast<char*>(data.buffer.data()), size))? FileResult::SUCCESS: FileResult::READ_FAILED;
}

FileResult writeBinaryFile(dataTransaction &tx, DataProcessor& data) {
    if (data.target.has_parent_path()) {std::filesystem::create_directories(data.target.parent_path());}
    std::ofstream outFile(data.target, std::ios::out|std::ios::binary);
    if (!outFile.is_open()) return FileResult::OPEN_FAILURE;
    std::println("WRITING FILE");
    outFile.write(reinterpret_cast<const char*>(data.buffer.data()), data.buffer.size());
    return outFile.good() ? FileResult::SUCCESS : FileResult::WRITE_FAILURE;
}
FileResult appendBinaryFile(dataTransaction &tx, DataProcessor& data) {
    if (data.target.has_parent_path()) {std::filesystem::create_directories(data.target.parent_path());}
    std::ofstream outFile(data.target, std::ios::out|std::ios::binary|std::ios::app);
    if (!outFile.is_open()) return FileResult::OPEN_FAILURE;
    std::println("WRITING FILE");
    outFile.write(reinterpret_cast<const char*>(data.buffer.data()), data.buffer.size());
    return outFile.good() ? FileResult::SUCCESS : FileResult::WRITE_FAILURE;
}

FileResult writeToFile(dataTransaction &tx, DataProcessor& data) {
    if (data.target.has_parent_path()) std::filesystem::create_directories(data.target.parent_path());
    std::ofstream outFile(data.target, std::ios::out);
    if (!outFile.is_open()) return FileResult::OPEN_FAILURE;
    outFile.write(reinterpret_cast<const char*>(data.buffer.data()), data.buffer.size());
    return outFile.good() ? FileResult::SUCCESS : FileResult::WRITE_FAILURE;
}

FileResult moveFile(dataTransaction &tx,DataProcessor& data){
  auto res = readBinaryFile(tx, data);
  if (res == FileResult::SUCCESS){
    DataProcessor temp{data.dest,data.buffer};
    res = writeBinaryFile(tx, temp);
    std::filesystem::remove_all(data.target);
  }
  return res;
}

FileResult appendFile(dataTransaction &tx,DataProcessor& data) {
    if (data.target.has_parent_path()) std::filesystem::create_directories(data.target.parent_path());
    std::ofstream outFile(data.target, std::ios::out|std::ios::app);
    if (!outFile.is_open()) return FileResult::OPEN_FAILURE;
    outFile.write(reinterpret_cast<const char*>(data.buffer.data()), data.buffer.size());
    return outFile.good() ? FileResult::SUCCESS : FileResult::WRITE_FAILURE;
}

FileResult sendBinaryToStdout(dataTransaction &tx, DataProcessor& data) {
    std::println("Streaming {} bytes directly to standard output:", data.buffer.size());
    for (auto byte: data.buffer) std::print("{:02X} ", byte);
    std::cout << "\n";
    return FileResult::SUCCESS;
}

FileResult sendBinaryToWebSocket(dataTransaction& tx,  DataProcessor& data) {
    if (!tx.ws.is_open()) return FileResult::WRITE_FAILURE;
    boost::beast::error_code ec;
    tx.ws.binary(true);
    tx.ws.write(boost::asio::buffer(data.buffer));
    return ec ? FileResult::WRITE_FAILURE : FileResult::SUCCESS;
}

FileResult readBinaryFromWebSocket(dataTransaction &tx, DataProcessor& data) {
    if (!tx.ws.is_open()) return FileResult::READ_FAILED;
    boost::beast::flat_buffer dynamic_buffer;
    boost::beast::error_code ec;
    tx.ws.binary(true);
    tx.ws.read(dynamic_buffer);
    if (ec) return FileResult::READ_FAILED;
    size_t bytes_received = dynamic_buffer.size();
    data.buffer.resize(bytes_received); 
    boost::asio::buffer_copy(boost::asio::buffer(data.buffer.data(), data.buffer.size()), dynamic_buffer.data());
    return FileResult::SUCCESS;
}
} // namespace data_ops


void handleDataSync(dataTransaction &tx, data::DataProcessor& data){
using namespace data;
using namespace data_ops;
std::error_code ec;

  switch (tx.cmd) {
  //Receive
    case OP::RX: { 
      dataLogBuilder.push_back(std::format("T:{:%Y-%m-%d %H:%M:%S} -- NOTIFY:Receiving file transfer initiated for target: {}",std::chrono::system_clock::now(), data.target));
      tx.fileResult = readBinaryFromWebSocket(tx, data);
      if (tx.fileResult == FileResult::SUCCESS) {
        tx.fileResult = writeBinaryFile(tx, data);
        tx.status = (tx.fileResult== FileResult::SUCCESS) ? data::Result::SUCCESS : data::Result::FAILURE;
      }else{tx.status = data::Result::FAILURE;}
      break;
  }

  //SEND
  case OP::TX: 
    dataLogBuilder.push_back(std::format("T:{:%Y-%m-%d %H:%M:%S} -- NOTIFY:Transfer initiated for target: {}",std::chrono::system_clock::now(), data.target));
    tx.fileResult = readBinaryFile(tx,data);
    if (tx.fileResult == FileResult::SUCCESS) tx.fileResult = sendBinaryToWebSocket(tx, data);
    tx.status = ( tx.fileResult == FileResult::SUCCESS) ? Result::SUCCESS: Result::FAILURE;
    break;

  //NEW FILE CREATION
  case OP::NEW: {
    dataLogBuilder.push_back(std::format("T:{:%Y-%m-%d %H:%M:%S} -- NOTIFY:New File Creation processing initiated {}",std::chrono::system_clock::now(),data.target));
    tx.fileResult= (bpy::utility::createEmptyFile(data.target))? FileResult::SUCCESS : FileResult::OPEN_FAILURE;
    tx.status = (tx.fileResult == FileResult::SUCCESS)? Result::SUCCESS : Result::FAILURE;
    break;
  }

  case OP::DEL:
    dataLogBuilder.push_back(std::format("T:{:%Y-%m-%d %H:%M:%S} -- NOTIFY:Delete processing initiated for target: {}",std::chrono::system_clock::now(), data.target));
    std::filesystem::remove_all(data.target, ec);
    tx.status = (!ec) ? Result::SUCCESS : Result::FAILURE;
    tx.fileResult = (tx.status == Result::SUCCESS)? FileResult::SUCCESS : FileResult::UNKNOWN_ERROR;
    break;

  case OP::CP: 
    dataLogBuilder.push_back(std::format("T:{:%Y-%m-%d %H:%M:%S} -- NOTIFY:Copying initiated for target: {}",std::chrono::system_clock::now(), data.target));
    tx.fileResult = readBinaryFile(tx,data);
    for (auto elem : data.buffer){std::cout <<"0x" << std::hex << elem; };
    if (tx.fileResult == FileResult::SUCCESS) tx.fileResult = data_ops::writeBinaryFile(tx, data);
    tx.status = ( tx.fileResult == FileResult::SUCCESS) ? Result::SUCCESS : Result::FAILURE;
    break;

  case OP::MV:
    dataLogBuilder.push_back(std::format("T:{:%Y-%m-%d %H:%M:%S} -- NOTIFY:Move initiated for target: {}",std::chrono::system_clock::now(), data.target));
    tx.fileResult = moveFile(tx,data);
    tx.status = (tx.fileResult==FileResult::SUCCESS) ? Result::SUCCESS : Result::FAILURE;
    break;
  
  case OP::AP:
    dataLogBuilder.push_back(std::format("T:{:%Y-%m-%d %H:%M:%S} -- NOTIFY:Appending to target: {}",std::chrono::system_clock::now(), data.target));
    tx.fileResult = appendFile(tx,data);
    break;
    
  case OP::NOP: 
    dataLogBuilder.push_back(std::format("T:{:%Y-%m-%d %H:%M:%S} -- NOTIFY:No OP Selected. Ignoring Transaction Request",std::chrono::system_clock::now()));
    tx.fileResult = FileResult::SUCCESS;
    tx.status = Result::SUCCESS;
    break;
  }
}
