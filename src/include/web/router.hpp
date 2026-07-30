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
void handleDataRequest(data::dataTransaction *request, std::filesystem::path _target, std::vector<uint8_t>& _buffer) ;
//void handleDataRequest(data::dataTransaction *request) ;
//extern const std::unordered_map<std::string, std::function<void(RequestVariant&, ws_stream&)>> functionTable;
//extern const std::unordered_map<std::string, std::function<void(RequestVariant*, ws_stream&)>> functionTable;
//extern const std::unordered_map<std::string, std::function<void(RequestVariant*, std::filesystem::path _target, std::vector<uint8_t>& _buffer)>> functionTable;

void dispatch(const std::string &cmd, RequestVariant &req, std::filesystem::path& _target, std::vector<uint8_t>& _buffer);
