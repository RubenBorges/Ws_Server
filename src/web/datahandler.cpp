#include <BPY/util.hpp>
#include <boost/beast.hpp>
#include <boost/asio.hpp>
#include <boost/beast/core.hpp>
#include <boost/beast/websocket.hpp>
#include <boost/asio/connect.hpp>
#include <boost/asio/ip/tcp.hpp>
#include <boost/beast/core/stream_traits.hpp>
#include <boost/asio/streambuf.hpp>
#include <string>
#include <web/datahandler.hpp>
#include <web/router.hpp>
#include <fstream>
#include <iostream>
#include <variant>
#include <format>
#include <chrono>
#include <print>

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
FileResult readBinaryFile(dataTransaction &tx, std::filesystem::path _target, std::vector<uint8_t>& _buffer) {
    if (!std::filesystem::is_regular_file(_target)) return FileResult::NOT_FOUND;
    
    std::ifstream file(_target, std::ios::binary | std::ios::ate);
    if (!file.is_open()) return FileResult::OPEN_FAILURE;

    std::streamsize size = file.tellg();
    file.seekg(0, std::ios::beg);
    _buffer.reserve(size);
    std::println("READING FILE");
    return ( file.read(reinterpret_cast<char*>(_buffer.data()), size))? FileResult::SUCCESS: FileResult::READ_FAILED;
}

FileResult writeBinaryFile(const dataTransaction &tx,std::filesystem::path _target, std::vector<uint8_t>& _buffer) {
    if (_target.has_parent_path()) {std::filesystem::create_directories(_target.parent_path());}
    std::ofstream outFile(_target, std::ios::binary);
    if (!outFile.is_open()) return FileResult::OPEN_FAILURE;
    std::println("WRITING FILE");
    outFile.write(reinterpret_cast<const char*>(_buffer.data()), _buffer.size());
    return outFile.good() ? FileResult::SUCCESS : FileResult::WRITE_FAILURE;
}

FileResult writeToFile(const dataTransaction &tx, std::filesystem::path _target, std::vector<uint8_t>& _buffer) {
    std::filesystem::path fullOut = _target;
    if (fullOut.has_parent_path()) {
        std::filesystem::create_directories(fullOut.parent_path());
    }
    std::ofstream outFile(fullOut, std::ios::binary);
    if (!outFile.is_open()) return FileResult::OPEN_FAILURE;

    outFile.write(reinterpret_cast<const char*>(_buffer.data()), _buffer.size());
    return outFile.good() ? FileResult::SUCCESS : FileResult::WRITE_FAILURE;
}

FileResult sendBinaryToStdout(const dataTransaction &tx, std::filesystem::path _target, std::vector<uint8_t>& _buffer) {
    std::cout << "Streaming " << _buffer.size() << " bytes directly to standard output:\n";
    for (size_t i = 0; i < _buffer.size(); ++i) {
        std::cout << "0x" << std::hex << static_cast<int>(_buffer[i]) << " ";
    }
    std::cout << "\n";
    return FileResult::SUCCESS;
}

FileResult sendBinaryToWebSocket(const dataTransaction& tx, std::filesystem::path _target, std::vector<uint8_t>& _buffer) {
    if (!tx.ws->is_open()) {
      std::println("ERROR: WebSocket Access FAILURE");
      return FileResult::WRITE_FAILURE;
    }
    
    //tx.ws.async_write(_buffer);
    boost::beast::error_code ec;
    std::stringstream ss; 
    std::string temp;
    for (auto elem : _buffer) ss<<(char*)elem;
    std::string my_str = std::move(ss).str(); 
    return ec ? FileResult::WRITE_FAILURE : FileResult::SUCCESS;
}

FileResult readBinaryFromWebSocket(dataTransaction &tx, std::filesystem::path _target, std::vector<uint8_t>& _buffer) {
    if (!tx.ws) return FileResult::READ_FAILED;
    boost::beast::flat_buffer dynamic_buffer;
    boost::beast::error_code ec;
    tx.ws->read(dynamic_buffer, ec);
    if (ec) return FileResult::READ_FAILED;
    size_t bytes_received = dynamic_buffer.size();
    _buffer.resize(bytes_received); 
    boost::asio::buffer_copy(boost::asio::buffer(_buffer.data(), _buffer.size()), dynamic_buffer.data());
    return FileResult::SUCCESS;
}
} // namespace data_ops


void handleDataSync(dataTransaction &tx, std::filesystem::path _target, std::vector<uint8_t>& _buffer){
using namespace data;
using namespace data_ops;
  switch (tx.cmd) {

  //Receive
    case OP::RX: { 
      // 1. Read the bytes coming out of the remote web socket pipeline matrix
      tx.fileResult = readBinaryFromWebSocket(tx, _target, _buffer);
      
      // 2. If network extraction finishes cleanly, commit the downloaded buffer to local disk blocks
      if (tx.fileResult == FileResult::SUCCESS) {
        tx.fileResult = writeBinaryFile(tx, _target, _buffer);
        tx.status = (tx.fileResult== FileResult::SUCCESS) ? data::Result::SUCCESS : data::Result::FAILURE;
      }else{tx.status = data::Result::FAILURE;}
      break;
  }

  //SEND
  case OP::TX: {
    tx.fileResult = readBinaryFile(tx, _target, _buffer);
    if (tx.fileResult == FileResult::SUCCESS) tx.fileResult = sendBinaryToWebSocket(tx, _target, _buffer);
    tx.status = ( tx.fileResult == FileResult::SUCCESS) ? Result::SUCCESS: Result::FAILURE;
    break;
  }

  //NEW FILE CREATION
  case OP::NEW: {
    dataLogBuilder.push_back(std::format(
        "T:{:%Y-%m-%d %H:%M:%S} -- NOTIFY:File Creation processing initiated.",std::chrono::system_clock::now()));
    auto res = bpy::utility::createEmptyFile(_target);
    res == true? tx.fileResult=FileResult::SUCCESS:tx.fileResult=FileResult::OPEN_FAILURE;
    tx.status = (tx.fileResult == FileResult::SUCCESS)? Result::SUCCESS : Result::FAILURE;
    break;
  }

  case OP::DEL: {
    dataLogBuilder.push_back(
        std::format("T:{:%Y-%m-%d %H:%M:%S} -- NOTIFY:Delete processing "
                    "initiated for target: {}",
                    std::chrono::system_clock::now(), _target));

    std::error_code ec; // Swapped to standard error code to preserve standalone
                        // compatibility boundaries
    std::filesystem::remove_all(_target, ec);
    tx.status = (!ec) ? Result::SUCCESS : Result::FAILURE;
    tx.fileResult = (tx.status == Result::SUCCESS)? FileResult::SUCCESS : FileResult::UNKNOWN_ERROR;
    break;
  }

  case OP::CP: 
    tx.fileResult = readBinaryFile(tx, _target, _buffer);
    for (auto elem : _buffer){std::cout <<"0x" << std::hex << elem; };
    if (tx.fileResult == FileResult::SUCCESS) tx.fileResult = data_ops::writeBinaryFile(tx, _target, _buffer);
    tx.status = ( tx.fileResult == FileResult::SUCCESS) ? Result::SUCCESS : Result::FAILURE;
    break;

  case OP::MV:{
    tx.fileResult = readBinaryFile(tx, _target, _buffer);
    std::error_code ec;
    if (tx.fileResult == FileResult::SUCCESS){
      tx.fileResult = data_ops::writeBinaryFile(tx, _target, _buffer);
      std::filesystem::remove_all(_target, ec);}
    else{
    tx.status = (!ec && tx.fileResult==FileResult::SUCCESS) ? Result::SUCCESS : Result::FAILURE;}
    break;}

  case OP::NOP: 
    std::println("No OP Selected. Ignoring Transaction Request");
    tx.fileResult = FileResult::SUCCESS;
    tx.status = Result::SUCCESS;
    break;

  default:
    std::println("Unknown Transaction Failure");
    tx.fileResult = FileResult::UNKNOWN_ERROR;
    tx.status = Result::FAILURE;
    break;
  }
}
