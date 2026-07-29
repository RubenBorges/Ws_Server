#pragma once 
#include <string>
#include <unordered_map>
#include <functional> 
#include <variant>     
#include <boost/asio/ip/tcp.hpp>
#include <boost/beast/websocket/stream.hpp>

#include "loginhandler.hpp" 
#include "datahandler.hpp"  

using ws_stream = boost::beast::websocket::stream<boost::asio::ip::tcp::socket>;

// 1. Explicitly match your exact target variant design pair
using RequestVariant = std::variant<loginRequest, data::dataTransaction>;

void handleLogin(loginRequest &req);
void handleDataRequest(data::dataTransaction &req); // Hand over mapping engine

extern const std::unordered_map<std::string, std::function<void(RequestVariant&, ws_stream&)>> functionTable;

void dispatch(const std::string &cmd, RequestVariant& req, ws_stream* ws);
