#include <web/datahandler.hpp>
#include <BPY/util.hpp>
#include <cstdio>
#include <dir_crawler.hpp>
#include <filesystem>
#include <fstream>
#include <print>

// Define and initialize state tables safely inside exactly one compiled target object
std::vector<std::string> dataLogBuilder;
std::vector<std::string> activeServerFilePaths;
data::dataTransaction ThisDataTransaction;

namespace data_ops {
// READ FROM FILE
data::FileResult readBinaryFile(data::dataTransaction &tx) {
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
    dataLogBuilder.push_back(
        std::format("T:{:%Y-%m-%d %H:%M:%S} -- ERROR:Failed to open File [{}].",
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

  // WRITE TO FILE
   data::FileResult writeBinaryFile(const data::dataTransaction &tx) {
    dataLogBuilder.push_back(std::format("T:{:%Y-%m-%d %H:%M:%S} -- NOTIFY:Writing data to File: {}",std::chrono::system_clock::now(),tx.targetPath));
    std::ofstream outFile(tx.targetPath, std::ios::binary);
    if (!outFile.is_open()){
      dataLogBuilder.push_back(std::format("T:{:%Y-%m-%d %H:%M:%S} -- ERROR:Failed to open File: {}",std::chrono::system_clock::now(),tx.targetPath));
      return data::FileResult::OPEN_FAILURE;
      }
    std::span<const uint8_t> dataSpan(tx.memoryBuffer);
    auto byteSpan = std::as_bytes(dataSpan);
    outFile.write(reinterpret_cast<const char *>(byteSpan.data()),byteSpan.size_bytes());
    dataLogBuilder.push_back(std::format("T:{:%Y-%m-%d %H:%M:%S} -- NOTIFY:Data successfully written to File: {}",std::chrono::system_clock::now(),tx.targetPath));
    return data::FileResult::SUCCESS;
   }

 data::FileResult writeToFile(const data::dataTransaction &tx ) {
   dataLogBuilder.push_back(
       std::format("T:{:%Y-%m-%d %H:%M:%S} -- NOTIFY:Writing data to File: {}",
                   std::chrono::system_clock::now(), tx.targetPath.string()));
   if (tx.targetPath.has_parent_path()) {
     std::filesystem::create_directories(tx.targetPath.parent_path());
   }
   std::ofstream outFile(tx.targetPath, std::ios::binary);
   if (!outFile.is_open()) {
     dataLogBuilder.push_back(std::format(
         "T:{:%Y-%m-%d %H:%M:%S} -- ERROR:Failed writing data to File: {}",
         std::chrono::system_clock::now(), tx.targetPath.string()));
     return data::FileResult::OPEN_FAILURE;
   }
   std::span<const uint8_t> dataSpan(tx.memoryBuffer);
   auto byteSpan = std::as_bytes(dataSpan);
   outFile.write(reinterpret_cast<const char *>(byteSpan.data()),
                 byteSpan.size_bytes());
   dataLogBuilder.push_back(std::format(
       "T:{:%Y-%m-%d %H:%M:%S} -- NOTIFY:Data successfully written to File: {}",
       std::chrono::system_clock::now(), tx.targetPath.string()));
   return outFile.good() ? data::FileResult::SUCCESS
                         : data::FileResult::READ_FAILED;
 }

// WRITE to stdout
data::FileResult sendBinaryToStdout(const data::dataTransaction &tx) {
  dataLogBuilder.push_back(
      std::format("T:{:%Y-%m-%d %H:%M:%S} -- NOTIFY:Writing data to stdout",
                  std::chrono::system_clock::now()));
  std::cout << "Sending " << tx.memoryBuffer.size()
            << " bytes of binary data...\n";

  for (size_t i = 0; i < tx.memoryBuffer.size(); ++i) {
    std::cout << "0x" << std::hex << static_cast<int>(tx.memoryBuffer[i])
              << " ";
  }
  std::cout << "\n";
  dataLogBuilder.push_back(std::format(
      "T:{:%Y-%m-%d %H:%M:%S} -- NOTIFY:Data successfully written to stdout.",
      std::chrono::system_clock::now()));
    return data::FileResult::SUCCESS;
}

// WRITE to WebSocket
data::FileResult sendBinaryToWebSocket(const data::dataTransaction &tx) {
  ws_stream* const ws{tx.socketContext};
  if (ws == nullptr) return data::FileResult::WRITE_FAILURE;
  dataLogBuilder.push_back(
      std::format("T:{:%Y-%m-%d %H:%M:%S} -- NOTIFY:Writing data to WebSocket",
                  std::chrono::system_clock::now()));
  std::cout << "Sending " << tx.memoryBuffer.size()
            << " bytes of binary data via WebSocket...\n";

  ws->binary(true);
  boost::beast::error_code ec;
  ws->write(boost::asio::buffer(tx.memoryBuffer.data(), tx.memoryBuffer.size()),
           ec);

  if (ec) {
    std::cerr << "WebSocket send failed: " << ec.message() << "\n";
    dataLogBuilder.push_back(std::format(
      "T:{:%Y-%m-%d %H:%M:%S} -- ERROR:Failed writing data to WebSocket",
    std::chrono::system_clock::now()));
    return data::FileResult::WRITE_FAILURE;}

    dataLogBuilder.push_back(
        std::format("T:{:%Y-%m-%d %H:%M:%S} -- NOTIFY:Data successfully "
                    "written to WebSocket",
                    std::chrono::system_clock::now()));
    return data::FileResult::SUCCESS;
}
// RECEIVE from WebSocket
data::FileResult readBinaryFromWebSocket(data::dataTransaction &tx) {
  ws_stream* const ws{tx.socketContext};
  if (ws == nullptr) return data::FileResult::READ_FAILED;

  dataLogBuilder.push_back(
      std::format("T:{:%Y-%m-%d %H:%M:%S} -- NOTIFY:Reading incoming file data from WebSocket",
                  std::chrono::system_clock::now()));

  // 1. Allocate a flat buffer to hold incoming network chunks
  boost::beast::flat_buffer dynamic_incoming_buffer;
  boost::beast::error_code ec;

  // 2. Read the next incoming network frame directly from this connection channel
  // Note: For production coroutines, match this to: co_await ws->async_read(...)
  ws->read(dynamic_incoming_buffer, ec);

  if (ec) {
    std::cerr << "WebSocket file reception failed: " << ec.message() << "\n";
    dataLogBuilder.push_back(std::format(
      "T:{:%Y-%m-%d %H:%M:%S} -- ERROR:Failed reading binary payload from WebSocket",
      std::chrono::system_clock::now()));
    return data::FileResult::READ_FAILED;
  }

  // 3. Clear any historical transaction debris and copy the bytes into our payload memory vector
  auto data_data = dynamic_incoming_buffer.data();
  const uint8_t* raw_bytes = reinterpret_cast<const uint8_t*>(data_data.data());
  size_t total_size = data_data.size();

  // Cast non-const state safely since this transaction object instance is mutable
  auto& mutable_tx = const_cast<data::dataTransaction&>(tx);
  mutable_tx.memoryBuffer.assign(raw_bytes, raw_bytes + total_size);

  std::cout << "Received " << tx.memoryBuffer.size() << " bytes of binary data via WebSocket.\n";
  
  dataLogBuilder.push_back(
      std::format("T:{:%Y-%m-%d %H:%M:%S} -- NOTIFY:Data successfully loaded into memoryBuffer from WebSocket. Size: {}B",
                  std::chrono::system_clock::now(), tx.memoryBuffer.size()));
                  
  return data::FileResult::SUCCESS;
}

} // namespace data_ops

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
    tx.status = (!ec && tx.fileResult==data::FileResult::SUCCESS) ? data::Result::SUCCESS : data::Result::FAILURE;
    break;}}

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
