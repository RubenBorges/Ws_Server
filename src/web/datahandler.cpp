#include <web/datahandler.hpp>
#include <BPY/util.hpp>
#include <fstream>
#include <iostream>
#include <format>
#include <chrono>
#include <print>
// Allocate your global variable storage targets exactly once here in memory
std::vector<std::string> dataLogBuilder;
std::vector<std::string> activeServerFilePaths;
data::dataTransaction ThisDataTransaction;

namespace data_ops {

data::FileResult readBinaryFile(data::dataTransaction &tx) {
    if (!std::filesystem::is_regular_file(tx.targetPath)) return data::FileResult::NOT_FOUND;
    
    std::ifstream file(tx.targetPath, std::ios::binary | std::ios::ate);
    if (!file.is_open()) return data::FileResult::OPEN_FAILURE;

    std::streamsize size = file.tellg();
    file.seekg(0, std::ios::beg);
    tx.memoryBuffer.resize(size);

    if (file.read(reinterpret_cast<char*>(tx.memoryBuffer.data()), size)) {
        return data::FileResult::SUCCESS;
    }
    return data::FileResult::READ_FAILED;
}

data::FileResult writeBinaryFile(const data::dataTransaction &tx) {
    if (tx.targetPath.has_parent_path()) {
        std::filesystem::create_directories(tx.targetPath.parent_path());
    }
    std::ofstream outFile(tx.targetPath, std::ios::binary);
    if (!outFile.is_open()) return data::FileResult::OPEN_FAILURE;

    outFile.write(reinterpret_cast<const char*>(tx.memoryBuffer.data()), tx.memoryBuffer.size());
    return outFile.good() ? data::FileResult::SUCCESS : data::FileResult::WRITE_FAILURE;
}

data::FileResult writeToFile(const data::dataTransaction &tx) {
    std::filesystem::path fullOut = tx.outputPath / tx.targetPath.filename();
    if (fullOut.has_parent_path()) {
        std::filesystem::create_directories(fullOut.parent_path());
    }
    std::ofstream outFile(fullOut, std::ios::binary);
    if (!outFile.is_open()) return data::FileResult::OPEN_FAILURE;

    outFile.write(reinterpret_cast<const char*>(tx.memoryBuffer.data()), tx.memoryBuffer.size());
    return outFile.good() ? data::FileResult::SUCCESS : data::FileResult::WRITE_FAILURE;
}

data::FileResult sendBinaryToStdout(const data::dataTransaction &tx) {
    std::cout << "Streaming " << tx.memoryBuffer.size() << " bytes directly to standard output:\n";
    for (size_t i = 0; i < tx.memoryBuffer.size(); ++i) {
        std::cout << "0x" << std::hex << static_cast<int>(tx.memoryBuffer[i]) << " ";
    }
    std::cout << "\n";
    return data::FileResult::SUCCESS;
}

data::FileResult sendBinaryToWebSocket(const data::dataTransaction &tx) {
    if (!tx.socketContext) return data::FileResult::WRITE_FAILURE;
    
    tx.socketContext->binary(true);
    boost::beast::error_code ec;
    tx.socketContext->write(boost::asio::buffer(tx.memoryBuffer.data(), tx.memoryBuffer.size()), ec);
    
    return ec ? data::FileResult::WRITE_FAILURE : data::FileResult::SUCCESS;
}

data::FileResult readBinaryFromWebSocket(data::dataTransaction &tx) {
    if (!tx.socketContext) return data::FileResult::READ_FAILED;

    boost::beast::flat_buffer dynamic_buffer;
    boost::beast::error_code ec;
    tx.socketContext->read(dynamic_buffer, ec);
    if (ec) return data::FileResult::READ_FAILED;

    auto data_view = dynamic_buffer.data();
    const uint8_t* raw_bytes = reinterpret_cast<const uint8_t*>(data_view.data());
    tx.memoryBuffer.assign(raw_bytes, raw_bytes + data_view.size());

    return data::FileResult::SUCCESS;
}

} // namespace data_ops


void handleDataSync(data::dataTransaction &tx, ::ws_stream& ws) {
    tx.socketContext = &ws;
    handleDataSync(tx);
}

void handleDataSync(data::dataTransaction &tx) {

  switch (tx.cmd) {

  //Receive
    case data::OP::RX: { 
      // 1. Read the bytes coming out of the remote web socket pipeline matrix
      tx.fileResult = data_ops::readBinaryFromWebSocket(tx);
      
      // 2. If network extraction finishes cleanly, commit the downloaded buffer to local disk blocks
      if (tx.fileResult == data::FileResult::SUCCESS) {
        tx.fileResult = data_ops::writeBinaryFile(tx);
        tx.status = (tx.fileResult== data::FileResult::SUCCESS) ? data::Result::SUCCESS : data::Result::FAILURE;
      }else{tx.status = data::Result::FAILURE;}
      break;
  }

  //SEND
  case data::OP::TX: {
    tx.fileResult = data_ops::readBinaryFile(tx);
    if (tx.fileResult == data::FileResult::SUCCESS) tx.fileResult = data_ops::sendBinaryToWebSocket(tx);
    tx.status = ( tx.fileResult == data::FileResult::SUCCESS) ? data::Result::SUCCESS: data::Result::FAILURE;
    break;
  }

  //NEW FILE CREATION
  case data::OP::NEW: {
    dataLogBuilder.push_back(std::format(
        "T:{:%Y-%m-%d %H:%M:%S} -- NOTIFY:File Creation processing initiated.",
        std::chrono::system_clock::now()));
    auto res = bpy::utility::createEmptyFile(tx.targetPath);
    res == true? tx.fileResult=data::FileResult::SUCCESS:tx.fileResult=data::FileResult::OPEN_FAILURE;
    tx.status = (tx.fileResult == data::FileResult::SUCCESS)? data::Result::SUCCESS : data::Result::FAILURE;
    break;
  }

  case data::OP::DEL: {
    dataLogBuilder.push_back(
        std::format("T:{:%Y-%m-%d %H:%M:%S} -- NOTIFY:Delete processing "
                    "initiated for target: {}",
                    std::chrono::system_clock::now(), tx.targetPath.string()));

    std::error_code ec; // Swapped to standard error code to preserve standalone
                        // compatibility boundaries
    std::filesystem::remove_all(tx.targetPath, ec);
    tx.status = (!ec) ? data::Result::SUCCESS : data::Result::FAILURE;
    tx.fileResult = (tx.status == data::Result::SUCCESS)? data::FileResult::SUCCESS : data::FileResult::UNKNOWN_ERROR;
    break;
  }

  case data::OP::CP: 
    tx.fileResult = data_ops::readBinaryFile(tx);
    if (tx.fileResult == data::FileResult::SUCCESS) tx.fileResult = data_ops::writeBinaryFile(tx);
    tx.status = ( tx.fileResult == data::FileResult::SUCCESS) ? data::Result::SUCCESS : data::Result::FAILURE;
    break;

  case data::OP::MV:{
    tx.fileResult = data_ops::readBinaryFile(tx);
    std::error_code ec;
    if (tx.fileResult == data::FileResult::SUCCESS){
      tx.fileResult = data_ops::writeBinaryFile(tx);
      std::filesystem::remove_all(tx.targetPath, ec);}
    else{
    tx.status = (!ec && tx.fileResult==data::FileResult::SUCCESS) ? data::Result::SUCCESS : data::Result::FAILURE;}
    break;}

  case data::OP::NOP: 
    std::println("No OP Selected. Ignoring Transaction Request");
    tx.fileResult = data::FileResult::SUCCESS;
    tx.status = data::Result::SUCCESS;
    break;

  default:
    std::println("Unknown Transaction Failure");
    tx.fileResult = data::FileResult::UNKNOWN_ERROR;
    tx.status = data::Result::FAILURE;
    break;
  }
}
