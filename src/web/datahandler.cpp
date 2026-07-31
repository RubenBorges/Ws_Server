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
#include <string>
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
// Allocate your global variable storage targets exactly once here in memory
std::vector<std::string> dataLogBuilder;
std::vector<std::string> activeServerFilePaths;

RequestVariant req;
namespace data_ops {
FileResult readBinaryFile(dataTransaction &tx, const std::filesystem::path* _target, std::vector<uint8_t>& _buffer) {
    if (!std::filesystem::is_regular_file(*_target)) return FileResult::NOT_FOUND;
    std::ifstream file(*_target, std::ios::in|std::ios::binary);
    if (!file.is_open()) return FileResult::OPEN_FAILURE;
    std::streamsize size = file.tellg();
    file.seekg(0, std::ios::beg);
    _buffer.reserve(size);
    std::println("READING FILE");
    return ( file.read(reinterpret_cast<char*>(_buffer.data()), size))? FileResult::SUCCESS: FileResult::READ_FAILED;
}

FileResult writeBinaryFile(const dataTransaction &tx,const std::filesystem::path* _target, std::vector<uint8_t>& _buffer) {
    if (_target->has_parent_path()) {std::filesystem::create_directories(_target->parent_path());}
    std::ofstream outFile(*_target, std::ios::out|std::ios::binary);
    if (!outFile.is_open()) return FileResult::OPEN_FAILURE;
    std::println("WRITING FILE");
    outFile.write(reinterpret_cast<const char*>(_buffer.data()), _buffer.size());
    return outFile.good() ? FileResult::SUCCESS : FileResult::WRITE_FAILURE;
}
FileResult appendBinaryFile(const dataTransaction &tx, const std::filesystem::path* _target, std::vector<uint8_t>& _buffer) {
    if (_target->has_parent_path()) {std::filesystem::create_directories(_target->parent_path());}
    std::ofstream outFile(*_target, std::ios::out|std::ios::binary|std::ios::app);
    if (!outFile.is_open()) return FileResult::OPEN_FAILURE;
    std::println("WRITING FILE");
    outFile.write(reinterpret_cast<const char*>(_buffer.data()), _buffer.size());
    return outFile.good() ? FileResult::SUCCESS : FileResult::WRITE_FAILURE;
}

FileResult writeToFile(const dataTransaction &tx, const std::filesystem::path* _target, std::vector<uint8_t>& _buffer) {
    if (_target->has_parent_path()) std::filesystem::create_directories(_target->parent_path());
    std::ofstream outFile(*_target, std::ios::out);
    if (!outFile.is_open()) return FileResult::OPEN_FAILURE;
    outFile.write(reinterpret_cast<const char*>(_buffer.data()), _buffer.size());
    return outFile.good() ? FileResult::SUCCESS : FileResult::WRITE_FAILURE;
}

FileResult moveFile(const dataTransaction &tx, const std::filesystem::path* _targets, std::vector<uint8_t>& _buffer){
auto res = readBinaryFile(tx, _targets,_buffer);
std::filesystem::path to = *(_targets+1);
return FileResult::SUCCESS;
}

FileResult appendFile(const dataTransaction &tx,const std::filesystem::path* _target, std::vector<uint8_t>& _buffer) {
    if (_target->has_parent_path()) std::filesystem::create_directories(_target->parent_path());
    std::ofstream outFile(*_target, std::ios::out|std::ios::app);
    if (!outFile.is_open()) return FileResult::OPEN_FAILURE;
    outFile.write(reinterpret_cast<const char*>(_buffer.data()), _buffer.size());
    return outFile.good() ? FileResult::SUCCESS : FileResult::WRITE_FAILURE;
}

FileResult sendBinaryToStdout(const dataTransaction &tx, std::vector<uint8_t>& _buffer) {
    std::println("Streaming {} bytes directly to standard output:", _buffer.size());
    for (auto byte: _buffer) std::print("{:02X} ", byte);
    std::cout << "\n";
    return FileResult::SUCCESS;
}

FileResult sendBinaryToWebSocket(const dataTransaction& tx, std::vector<uint8_t>& _buffer) {
    if (!tx.ws->is_open()) return FileResult::WRITE_FAILURE;
    boost::beast::error_code ec;
    tx.ws->binary(true);
    tx.ws->write(_buffer,ec);
    return ec ? FileResult::WRITE_FAILURE : FileResult::SUCCESS;
}

FileResult readBinaryFromWebSocket(dataTransaction &tx, std::vector<uint8_t>& _buffer) {
    if (!tx.ws) return FileResult::READ_FAILED;
    boost::beast::flat_buffer dynamic_buffer;
    boost::beast::error_code ec;
    tx.ws->binary(true);
    tx.ws->read(_buffer, ec);
    if (ec) return FileResult::READ_FAILED;
    size_t bytes_received = dynamic_buffer.size();
    _buffer.resize(bytes_received); 
    boost::asio::buffer_copy(boost::asio::buffer(_buffer.data(), _buffer.size()), dynamic_buffer.data());
    return FileResult::SUCCESS;
}
} // namespace data_ops


void handleDataSync(dataTransaction &tx, const std::filesystem::path* _target, std::vector<uint8_t>& _buffer){
using namespace data;
using namespace data_ops;
std::error_code ec;

  switch (tx.cmd) {
  //Receive
    case OP::RX: { 
      tx.fileResult = readBinaryFromWebSocket(tx,  _buffer);
      if (tx.fileResult == FileResult::SUCCESS) {
        tx.fileResult = writeBinaryFile(tx, _target, _buffer);
        tx.status = (tx.fileResult== FileResult::SUCCESS) ? data::Result::SUCCESS : data::Result::FAILURE;
      }else{tx.status = data::Result::FAILURE;}
      break;
  }

  //SEND
  case OP::TX: 
    tx.fileResult = readBinaryFile(tx, _target, _buffer);
    if (tx.fileResult == FileResult::SUCCESS) tx.fileResult = sendBinaryToWebSocket(tx, _buffer);
    tx.status = ( tx.fileResult == FileResult::SUCCESS) ? Result::SUCCESS: Result::FAILURE;
    break;

  //NEW FILE CREATION
  case OP::NEW: {
    dataLogBuilder.push_back(std::format("T:{:%Y-%m-%d %H:%M:%S} -- NOTIFY:File Creation processing initiated {}",std::chrono::system_clock::now(),*_target));
    tx.fileResult= (bpy::utility::createEmptyFile(*_target))? FileResult::SUCCESS : FileResult::OPEN_FAILURE;
    tx.status = (tx.fileResult == FileResult::SUCCESS)? Result::SUCCESS : Result::FAILURE;
    break;
  }

  case OP::DEL:
    dataLogBuilder.push_back(std::format("T:{:%Y-%m-%d %H:%M:%S} -- NOTIFY:Delete processing initiated for target: {}",std::chrono::system_clock::now(), *_target));
    std::filesystem::remove_all(*_target, ec);
    tx.status = (!ec) ? Result::SUCCESS : Result::FAILURE;
    tx.fileResult = (tx.status == Result::SUCCESS)? FileResult::SUCCESS : FileResult::UNKNOWN_ERROR;
    break;

  case OP::CP: 
    tx.fileResult = readBinaryFile(tx, _target, _buffer);
    for (auto elem : _buffer){std::cout <<"0x" << std::hex << elem; };
    if (tx.fileResult == FileResult::SUCCESS) tx.fileResult = data_ops::writeBinaryFile(tx, _target, _buffer);
    tx.status = ( tx.fileResult == FileResult::SUCCESS) ? Result::SUCCESS : Result::FAILURE;
    break;

  case OP::MV:
    tx.fileResult = readBinaryFile(tx, _target, _buffer);
    if (tx.fileResult == FileResult::SUCCESS){
      tx.fileResult = data_ops::writeBinaryFile(tx, _target, _buffer);
      std::filesystem::remove_all(*_target, ec);}
    else{tx.status = (!ec && tx.fileResult==FileResult::SUCCESS) ? Result::SUCCESS : Result::FAILURE;}
    break;
  
  case OP::AP:
    tx.fileResult = appendFile(tx, _target, _buffer);
    break;
    
  case OP::NOP: 
    std::println("No OP Selected. Ignoring Transaction Request");
    tx.fileResult = FileResult::SUCCESS;
    tx.status = Result::SUCCESS;
    break;
  }
}
