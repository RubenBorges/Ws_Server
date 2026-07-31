#pragma once 
#include <string>
#include <variant> 
#include <optional>     
#include <boost/asio/ip/tcp.hpp>
#include <boost/beast/websocket/stream.hpp>
#include <boost/asio/io_context.hpp>
#include "loginhandler.hpp" 
#include "datahandler.hpp"  

using ws_stream = boost::beast::websocket::stream<boost::asio::ip::tcp::socket>;
using RequestVariant = std::variant<loginRequest, data::dataTransaction>;
namespace router{
void handleLogin(loginRequest &req);

void handleDataRequest(data::dataTransaction *request, data::DataProcessor& data) ;
void dispatch(const std::string& cmd, RequestVariant &req, data::DataProcessor* data);
void dispatch(const std::string& cmd, RequestVariant &req, const std::filesystem::path* _target, std::vector<uint8_t>& _buffer) ;
}