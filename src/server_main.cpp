#include <boost/asio/io_context.hpp>

#include <web/server.hpp>

#include <exception>
#include <iostream>
#include <string>

int main(int argc, char** argv) {
    namespace asio = boost::asio;

    unsigned short port = 4544;
    if (argc > 1) {
        try {
            port = static_cast<unsigned short>(std::stoi(argv[1]));
        } catch (const std::exception&) {
            std::cerr << "Invalid port: " << argv[1] << "\n";
            return 1;
        }
    }

    asio::io_context io(1);
    auto server = p2p::web::make_server(io, p2p::web::ServerOptions{port, "/shutdown"});
    server->start();

    std::cout << "Server listening on port " << port << ". Send /shutdown from client to stop.\n";
    io.run();
    return 0;
}
