#include <boost/asio.hpp>
#include <boost/asio/awaitable.hpp>
#include <boost/asio/use_awaitable.hpp>
#include <string>
#include <iostream>

namespace asio = boost::asio;
using asio::awaitable;
//using asio::use_awaitable;
using tcp = asio::ip::tcp;

using iocontext = boost::asio::io_context;

awaitable<void> run_client(iocontext &ctx, const std::string &host,
                           uint16_t port);


// Generic wrapper function
template <typename... Args>
void co_spawn(Args&&... args) {
    // Perform wrapper logic here (e.g., logging, timing)
    std::cout << "Wrapper logic executed..." << std::endl;

    // Perfect forward the arguments to the target
    asio::co_spawn(std::forward<Args>(args)...);
}