#pragma once 
#include <variant> 
#include <boost/asio/ip/tcp.hpp>
#include <boost/beast/websocket/stream.hpp>
#include <boost/asio/io_context.hpp>
#include <web/datahandler.hpp>
#include <web/loginhandler.hpp>

using ws_stream = boost::beast::websocket::stream<boost::asio::ip::tcp::socket>;
using RequestVariant = std::variant<loginRequest, data::dataTransaction>;

enum class RequestType : int { LOGIN = 0, DATA = 1 };
namespace router{
    int dispatch(RequestType cmd,std::variant<loginRequest, data::dataTransaction>& req);
    int DispatchRequest(RequestType reqType, RequestVariant &payload);
} // namespace router