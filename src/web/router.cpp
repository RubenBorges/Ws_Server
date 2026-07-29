#include <web/router.hpp>
#include <print>       
#include <iostream>    
#include <boost/asio/ip/tcp.hpp>
#include <boost/beast/websocket/stream.hpp>
void handleLogin(loginRequest &req) {
    login(req);
}

// Routes direct to your specific data handler engine
void handleDataRequest(data::dataTransaction &req) {
    handleDataSync(req); 
}

const std::unordered_map<std::string, std::function<void(RequestVariant &, ws_stream&)>> functionTable = {
    { "login", [](RequestVariant &req, ws_stream& ws) {
        if (auto *loginPtr = std::get_if<loginRequest>(&req)) {
            handleLogin(*loginPtr);
        } else {
            std::println(std::cerr, "Routing Error: Expected loginRequest payload.");
        }
    }},
    { "data", [](RequestVariant &req, ws_stream& ws) {
        if (auto *dataTX = std::get_if<data::dataTransaction>(&req)) {
            dataTX->socketContext = &ws;
            handleDataRequest(*dataTX);
        }else {std::println(std::cerr, "Routing Error: Expected dataRequest payload.");}
        }
    }
};

void dispatch(const std::string &cmd, RequestVariant &req, ws_stream* ws) {
    auto it = functionTable.find(cmd);
    if (it == functionTable.end()) {
        std::println(std::cerr, "Command: '{}' not supported.", cmd);
        return;
    }

    if (ws != nullptr) {
        it->second(req, *ws);
        return;
    }

    if (cmd == "login") {
        if (auto *loginPtr = std::get_if<loginRequest>(&req)) {
            handleLogin(*loginPtr);
        } else {
            std::println(std::cerr, "Routing Error: Expected loginRequest payload.");
        }
    } else if (cmd == "data") {
        if (auto *dataTX = std::get_if<data::dataTransaction>(&req)) {
            dataTX->socketContext = nullptr;
            handleDataRequest(*dataTX);
        } else {
            std::println(std::cerr, "Routing Error: Expected dataRequest payload.");
        }
    } else {
        std::println(std::cerr, "Command: '{}' not supported.", cmd);
    }
}
