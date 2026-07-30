#pragma once 
#include <string>
#include <variant>     
#include <boost/asio/ip/tcp.hpp>
#include <boost/beast/websocket/stream.hpp>
#include <boost/asio/io_context.hpp>
#include "loginhandler.hpp" 
#include "datahandler.hpp"  

using ws_stream = boost::beast::websocket::stream<boost::asio::ip::tcp::socket>;
using RequestVariant = std::variant<loginRequest, data::dataTransaction>;

void handleLogin(loginRequest &req);
void handleDataRequest(data::dataTransaction *request, std::filesystem::path _target, std::vector<uint8_t>& _buffer) ;
void dispatch(const std::string &cmd, RequestVariant &req, std::filesystem::path& _target, std::vector<uint8_t>& _buffer);
