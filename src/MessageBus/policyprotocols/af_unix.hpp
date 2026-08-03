#include <boost/asio.hpp>
#include <iostream>
#include <string>
#include <memory>
#include <system_error>

namespace asio = boost::asio;
using stream_protocol = asio::local::stream_protocol;

// 1. Enum to define the operation mode of the policy
enum class SocketRole {
    Client,
    Server
};

// 2. The Policy Class defining how the AF_UNIX interface behaves
class AsioUnixSocketPolicy {
public:
    // Configures and initializes the underlying OS socket interface based on the policy role
    static std::unique_ptr<stream_protocol::socket> configure_socket(
        asio::io_context& io_ctx, 
        const std::string& socket_path, 
        SocketRole role,
        boost::system::error_code& ec) 
    {
        auto socket = std::make_unique<stream_protocol::socket>(io_ctx);
        stream_protocol::endpoint endpoint(socket_path);

        if (role == SocketRole::Client) {
            // Client Policy: Connect directly to the existing path
            socket->connect(endpoint, ec);
        } 
        else if (role == SocketRole::Server) {
            // Server Policy: Clean up old file paths, bind, and prepare to accept
            // Unlink handles cleaning up old crashes so bind doesn't fail
            ::unlink(socket_path.c_str()); 

            // Create an acceptor to bind the address to the OS kernel
            stream_protocol::acceptor acceptor(io_ctx, endpoint);
            
            // Accept the next incoming client connection directly into our socket
            acceptor.accept(*socket, ec);
        }

        if (ec) {
            return nullptr; // Return null if the OS failed to provision the socket interface
        }

        return socket;
    }
};
/*
//SERVER
int main() {
    asio::io_context io_ctx;
    std::string path = "/tmp/policy_socket.sock";
    boost::system::error_code ec;

    std::cout << "Server execution: Waiting for a client to connect...\n";
    
    // Execute the Server variation of the interface policy
    auto server_socket = AsioUnixSocketPolicy::configure_socket(
        io_ctx, path, SocketRole::Server, ec
    );

    if (ec) {
        std::cerr << "Server policy execution failed: " << ec.message() << "\n";
        return 1;
    }

    // You now have an open, active bidirectional socket ready for communication
    std::cout << "Client successfully connected to server socket interface!\n";
    return 0;
}

//CLIENT
int main() {
    asio::io_context io_ctx;
    std::string path = "/tmp/policy_socket.sock";
    boost::system::error_code ec;

    std::cout << "Client execution: Connecting to server path...\n";

    // Execute the Client variation of the interface policy
    auto client_socket = AsioUnixSocketPolicy::configure_socket(
        io_ctx, path, SocketRole::Client, ec
    );

    if (ec) {
        std::cerr << "Client policy execution failed: " << ec.message() << "\n";
        return 1;
    }

    std::cout << "Connected to the server successfully via policy.\n";
    return 0;
}

*/