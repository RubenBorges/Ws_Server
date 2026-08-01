#pragma once 
#include <variant> 
#include <boost/asio/ip/tcp.hpp>
#include <boost/beast/websocket/stream.hpp>
#include <boost/asio/io_context.hpp>
#include "loginhandler.hpp" 
#include "datahandler.hpp"  

using ws_stream = boost::beast::websocket::stream<boost::asio::ip::tcp::socket>;
using RequestVariant = std::variant<loginRequest, data::dataTransaction>;


enum class RequestType : int { LOGIN = 0, DATA = 1 };
namespace router{
void handleLogin(loginRequest &req);
void handleDataRequest(data::dataTransaction& request) ;
}
void dispatch(RequestType cmd, std::variant<loginRequest, data::dataTransaction> &req);