#pragma once 

#include <string>
#include <unordered_map>
#include <functional> 
#include <variant>     
#include <boost/asio/ip/tcp.hpp>
#include <boost/beast/websocket/stream.hpp>

#include "loginhandler.hpp" 
#include "datahandler.hpp"  

// Ensure the websocket stream type exactly matches the server definition
using ws_stream = boost::beast::websocket::stream<boost::asio::ip::tcp::socket>;
using RequestVariant = std::variant<loginRequest, data::dataTransaction>;

void handleLogin(loginRequest &req);
void handleData(data::dataTransaction &tx);

// Declared with the unified ws_stream type
extern const std::unordered_map<std::string, std::function<void(RequestVariant &)>> functionTable;

void dispatch(const std::string &cmd, RequestVariant &req, ws_stream* ws = nullptr);