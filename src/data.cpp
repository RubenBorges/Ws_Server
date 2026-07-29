#include <algorithm>
#include <boost/random/random_device.hpp>
#include <boost/uuid/uuid.hpp>
#include <boost/uuid/uuid_generators.hpp>
#include <boost/uuid/uuid_io.hpp>
#include <chrono>
#include <BPY/util.hpp>
#include "BPY/DirectoryCrawler.hpp"
#include "dir_crawler.hpp"
#include "FBP_Tree.hpp"
#include "include/spanningtree.hpp"
#include <cstdint>
#include <cstdlib>
#include <format>
#include <fstream>
#include <iostream>
#include <jwt-cpp/jwt.h>
#include <print>
#include <span>
#include <string>
#include <unordered_map>
#include <vector>
#include <filesystem>

namespace data {
enum class OP : int { NOP = 0, TX = 1, RX = 2, NEW = 3, DEL = 4 };
enum class Result : int { SUCCESS = 0, FAILURE = 1, PENDING = 2 };

struct dataRequest {
    std::vector<std::string>& filepaths;
    OP op{OP::NOP};                 // FIXED: Explicitly use the enum class enumerator
    Result result{Result::PENDING}; // FIXED: Explicitly use the enum class enumerator
};

std::vector<uint8_t> readBinaryFile(const std::string& filePath) { 
    std::ifstream file(filePath, std::ios::binary | std::ios::ate);
    if (!file.is_open()) {
        std::cerr << "Failed to open file: " << filePath << "\n";
        return {};
    }
    std::streamsize size = file.tellg(); 
    file.seekg(0, std::ios::beg);
    std::vector<uint8_t> buffer(size);  
    if (file.read(reinterpret_cast<char*>(buffer.data()), size)) { 
        return buffer;
    }
    
    return {};
}

// Accepts std::filesystem::path directly
std::vector<uint8_t> readBinaryFile(const std::filesystem::path& filePath) { 
    // 1. Verify file exists and is not a folder before trying to read
    if (!std::filesystem::is_regular_file(filePath)) {
        std::cerr << "Path is not a valid file: " << filePath << "\n";
        return {};
    }

    std::ifstream file(filePath, std::ios::binary | std::ios::ate);
    if (!file.is_open()) {
        std::cerr << "Failed to open file: " << filePath << "\n";
        return {};
    }
    
    std::streamsize size = file.tellg(); 
    file.seekg(0, std::ios::beg);
    std::vector<uint8_t> buffer(size);  
    
    if (file.read(reinterpret_cast<char*>(buffer.data()), size)) { 
        return buffer;
    }
    return {};
}

void sendBinaryData(const uint8_t* data, size_t size) { 
    std::cout << "Sending " << size << " bytes of binary data...\n";
    
    for (size_t i = 0; i < std::min(size, size_t(5)); ++i) { 
        std::cout << "0x" << std::hex << static_cast<int>(data[i]) << " ";
    }
    std::cout << "\n";
}

bool writeBinaryFile(const std::string& path, const std::vector<uint8_t>& buffer) {
    std::ofstream outFile(path, std::ios::binary);
    if (!outFile.is_open()) return false;

    std::span<const uint8_t> dataSpan(buffer);
    auto byteSpan = std::as_bytes(dataSpan);
    outFile.write(reinterpret_cast<const char*>(byteSpan.data()), byteSpan.size_bytes());

    return outFile.good();
}

// Accepts std::filesystem::path directly
bool writeBinaryFile(const std::filesystem::path& path, const std::vector<uint8_t>& buffer) {
    // 2. Safely extract and create the parent folders if they don't exist yet
    if (path.has_parent_path()) {
        std::filesystem::create_directories(path.parent_path());
    }

    std::ofstream outFile(path, std::ios::binary);
    if (!outFile.is_open()) return false;

    std::span<const uint8_t> dataSpan(buffer);
    auto byteSpan = std::as_bytes(dataSpan);
    outFile.write(reinterpret_cast<const char*>(byteSpan.data()), byteSpan.size_bytes());

    return outFile.good();
}

} // FIXED: Removed trailing semicolon from namespace closure
int main(int argc, char* argv[]) {
    std::string targetroot{argc > 1 ? argv[1] : "/home/boopy/Pictures"}; 
    
    // Set up paths as filesystem objects
    std::filesystem::path copyfile{"/home/boopy/Pictures/out"};
    std::filesystem::path destfile{"/home/boopy/Projects/ws_server/writetest.jpg"};
    std::vector<std::string> imglist {"IMAGE FILES:"}, audiolist{"AUDIO FILES:"}, videolist{"VIDEOS FILES:"};
    imglist.reserve(100); audiolist.reserve(100); videolist.reserve(100);
    
    DirectoryCrawler dr(targetroot);
    std::vector<std::string> filepaths;
    dr.crawlRecursively(filepaths);

    std::cout << "\n--- Scanned JPEG Files ---\n";
    for (const auto& path_str : filepaths) {
        std::filesystem::path p(path_str);
        // Filter out everything except specific extensions
        if (p.extension() == ".jpg" || p.extension() == ".jpeg" || p.extension() == ".png") {
            imglist.emplace_back(p);
            std::cout << "Found image file: " << p.filename() << "\n";
            
        }
        if (p.extension() == ".wav" || p.extension() == ".mp3" || p.extension() == ".aac"|| p.extension() == ".m4a"|| p.extension() == ".flac"|| p.extension() == ".ogg"|| p.extension() == ".wma"|| p.extension() == ".opus") {
            audiolist.emplace_back(p);
            std::cout << "Found audio file: " << p.filename() << "\n";
        }
        if (p.extension() == ".mp4" || p.extension() == ".mkv" || p.extension() == ".avi" || p.extension() == ".mov" || p.extension() == "wmv" || p.extension() == ".flv" || p.extension() == ".webm" || p.extension() == ".m4v") {
            videolist.emplace_back(p);
            std::cout << "Found Video file: " << p.filename() << "\n";
        }
    }
    std::vector<std::vector<std::string>*> mediaList = {&imglist,&audiolist,&videolist};
    for (auto f : mediaList){
        for (auto i: *f){
            std::cout<<i<<"\n";
        }
    };
    // 3. Process the file copy
    std::vector<uint8_t> fileBuffer = data::readBinaryFile(copyfile);
    if (!fileBuffer.empty()) {
        data::writeBinaryFile(destfile, fileBuffer);
    }
    
    return EXIT_SUCCESS;
 }
// int main(int argc, char* argv[]) {
//     std::vector<std::string> filepaths;
//     filepaths.reserve(100);
//     std::string targetroot{argc > 1 ? argv[1] : "/home/boopy/dev/Projects/ws_server"};      
//     std::string copyfile{"/home/boopy/Pictures/out"};
//     static data::dataRequest req{filepaths, data::OP::NEW};

//     DirectoryCrawler dr(targetroot);
//     dr.crawlRecursively(filepaths);
//     for (const auto& e : filepaths) std::cout << e << "\n";
    
//     std::vector<uint8_t> fileBuffer = data::readBinaryFile(std::filesystem::path{copyfile});
    
//     // FIXED: Swapped out standard tilde (~) shortcut for absolute path so fstream can find it
//     data::writeBinaryFile("/home/boopy/Projects/ws_server/writetest.jpg", fileBuffer);
    
//     return EXIT_SUCCESS;
// }
