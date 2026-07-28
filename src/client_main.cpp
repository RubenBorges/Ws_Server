#include <boost/asio/co_spawn.hpp>
#include <boost/asio/executor_work_guard.hpp>
#include <boost/asio/io_context.hpp>
#include <boost/asio/use_future.hpp>

#include <web/client.hpp>

#include <exception>
#include <iostream>
#include <string>
#include <thread>

int main(int argc, char** argv) {
    namespace asio = boost::asio;

    std::string host = "127.0.0.1";
    unsigned short port = 4544;

    if (argc > 1) {
        host = argv[1];
    }
    if (argc > 2) {
        try {
            port = static_cast<unsigned short>(std::stoi(argv[2]));
        } catch (const std::exception&) {
            std::cerr << "Invalid port: " << argv[2] << "\n";
            return 1;
        }
    }

    asio::io_context io(1);
    auto work = asio::make_work_guard(io);
    std::thread io_thread([&io]() { io.run(); });

    try {
        p2p::web::AsyncEchoClient client(io);

        auto connect_future = asio::co_spawn(
            io,
            client.connect(p2p::web::ClientOptions{host, port}),
            asio::use_future);
        connect_future.get();

        std::cout << "Connected to " << host << ":" << port
                  << ". Type lines to echo. Send /shutdown to stop server.\n";

        std::string line;
        while (std::getline(std::cin, line)) {
            if (line.empty()) {
                continue;
            }

            auto reply_future = asio::co_spawn(io, client.send_and_receive(line), asio::use_future);
            std::cout << "Server: " << reply_future.get() << "\n";

            if (line == "/shutdown") {
                break;
            }
        }

        client.close();
    } catch (const std::exception& e) {
        std::cerr << "Client error: " << e.what() << "\n";
        work.reset();
        io.stop();
        io_thread.join();
        return 1;
    }

    work.reset();
    io.stop();
    io_thread.join();
    return 0;
}
