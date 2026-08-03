#include <boost/asio.hpp>
#include <fcntl.h>
#include <ios>
#include <sys/stat.h>
#include <iostream>
#include <sys/types.h>
#include <fstream>
#include "pstream/pstream.hpp"
#include <iostream>

namespace asio = boost::asio;

asio::posix::stream_descriptor NamedPipe(std::string& fifo_path, asio::io_context& io_ctx) {
    if(fifo_path.empty()) fifo_path= "/tmp/boost_fifo";

    mkfifo(fifo_path.c_str(), 0666);
    
    int fd = open(fifo_path.c_str(), O_RDONLY | O_NONBLOCK); // Open the FIFO using standard OS flags

    asio::posix::stream_descriptor fifo_stream(io_ctx, fd);
    
    // Now you can perform non-blocking async reads/writes using fifo_stream
    std::cout << "Boost Named Pipe descriptor ready for async I/O.\n";

    return std::move(fifo_stream);
}

int opstream(std::string* fifo_path, bool end = false){
std::fstream pstream;
if (fifo_path == nullptr) *fifo_path = "/tmp/my_named_pipe";

    // 1. Create the Named Pipe (FIFO) in the file system
    // Returns 0 on success, -1 if it already exists
    auto result = mkfifo(fifo_path->c_str(), 0666); 

    // ==========================================
    // WRITER WORKFLOW (e.g., Process A)
    // ==========================================
    std::ofstream writer(fifo_path->c_str()); // Blocks until a reader opens it
    if (writer.is_open()) {
        writer << "Data sent via Named Pipe!\n";
        writer.close();
    }

    // Cleanup file system when completely done
    if (end == true) unlink(fifo_path->c_str());
    return result;
}


int ipstream(std::string* fifo_path, bool end = false) {
    std::fstream pstream;
    if (fifo_path == nullptr) *fifo_path = "/tmp/my_named_pipe";

    auto result = mkfifo(fifo_path->c_str(), 0666); 
    
    // ==========================================
    // READER WORKFLOW (e.g., Process B)
    // ==========================================
    
    std::ifstream reader(fifo_path->c_str());
    std::string message;
    if (std::getline(reader, message)) {
        std::cout << "Received: " << message << "\n";
    }
    
    // Cleanup file system when completely done
    if (end == true) unlink(fifo_path->c_str()); 
    
    return result;
}
