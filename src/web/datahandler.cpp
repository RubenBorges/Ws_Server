#include "web/datahandler.hpp"
#include <dir_crawler.hpp>
#include <fstream>
// Define and initialize state tables safely inside exactly one compiled target
// object
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

// data::FileResult writeBinaryFile(const data::dataTransaction &tx) {
//   dataLogBuilder.push_back(
//       std::format("T:{:%Y-%m-%d %H:%M:%S} -- NOTIFY:Writing data to File: {}",
//                   std::chrono::system_clock::now(), tx.targetPath.string()));

//   if (tx.targetPath.has_parent_path()) {
//     std::filesystem::create_directories(tx.targetPath.parent_path());
//   }

//   std::ofstream outFile(tx.targetPath, std::ios::binary);
//   if (!outFile.is_open()) {
//     dataLogBuilder.push_back(std::format(
//         "T:{:%Y-%m-%d %H:%M:%S} -- ERROR:Failed writing data to File: {}",
//         std::chrono::system_clock::now(), tx.targetPath.string()));
//     return data::FileResult::OPEN_FAILURE;
//   }

//   std::span<const uint8_t> dataSpan(tx.memoryBuffer);
//   auto byteSpan = std::as_bytes(dataSpan);
//   outFile.write(reinterpret_cast<const char *>(byteSpan.data()),
//                 byteSpan.size_bytes());

//   dataLogBuilder.push_back(std::format(
//       "T:{:%Y-%m-%d %H:%M:%S} -- NOTIFY:Data successfully written to File: {}",
//       std::chrono::system_clock::now(), tx.targetPath.string()));
//   return outFile.good() ? data::FileResult::SUCCESS
//                         : data::FileResult::READ_FAILED;
// }

// WRITE to stdout
void sendBinaryToStdout(const data::dataTransaction &tx) {
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
}

// WRITE to WebSocket
void sendBinaryToWebSocket(ws_stream &ws, const data::dataTransaction &tx) {
  dataLogBuilder.push_back(
      std::format("T:{:%Y-%m-%d %H:%M:%S} -- NOTIFY:Writing data to WebSocket",
                  std::chrono::system_clock::now()));
  std::cout << "Sending " << tx.memoryBuffer.size()
            << " bytes of binary data via WebSocket...\n";

  ws.binary(true);
  boost::beast::error_code ec;
  ws.write(boost::asio::buffer(tx.memoryBuffer.data(), tx.memoryBuffer.size()),
           ec);

  if (ec) {
    std::cerr << "WebSocket send failed: " << ec.message() << "\n";
    dataLogBuilder.push_back(std::format(
        "T:{:%Y-%m-%d %H:%M:%S} -- ERROR:Failed writing data to WebSocket",
        std::chrono::system_clock::now()));
  } else {
    dataLogBuilder.push_back(
        std::format("T:{:%Y-%m-%d %H:%M:%S} -- NOTIFY:Data successfully "
                    "written to WebSocket",
                    std::chrono::system_clock::now()));
  }
}

} // namespace data_ops

void handleDataSync(data::dataTransaction &tx) {
  switch (tx.cmd) {
  case data::OP::RX: {
    data::FileResult res = data_ops::readBinaryFile(tx);
    tx.status = (res == data::FileResult::SUCCESS) ? data::Result::SUCCESS
                                                   : data::Result::FAILURE;
    break;
  }

  case data::OP::TX: {
    data::FileResult res = data_ops::writeBinaryFile(tx);
    tx.status = (res == data::FileResult::SUCCESS) ? data::Result::SUCCESS
                                                   : data::Result::FAILURE;
    break;
  }

  case data::OP::NEW: {
    dataLogBuilder.push_back(std::format(
        "T:{:%Y-%m-%d %H:%M:%S} -- NOTIFY:Crawl processing initiated.",
        std::chrono::system_clock::now()));

    DirectoryCrawler dr(tx.targetPath.string());
    dr.crawlRecursively(activeServerFilePaths);
    tx.status = data::Result::SUCCESS;
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
    break;
  }
  case data::OP::CP:
  case data::OP::MV:

  case data::OP::NOP:
  default:
    tx.status = data::Result::FAILURE;
    break;
  }
}
