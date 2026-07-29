#include <web/router.hpp>
#include <print>       
#include <iostream>    

void handleLogin(loginRequest &req) {
    login(req);
}

void handleData(data::dataTransaction &tx) {
    handleDataSync(tx);
}

const std::unordered_map<std::string, std::function<void(RequestVariant &)>> functionTable = {
    { "login", [](RequestVariant &req) {
        if (auto *loginPtr = std::get_if<loginRequest>(&req)) {
            handleLogin(*loginPtr);
        } else {
            std::println(std::cerr, "Routing Error: 'login' path expected loginRequest payload.");
        }
    }},
    { "data", [](RequestVariant &req) {
        if (auto *dataPtr = std::get_if<data::dataTransaction>(&req)) {
            handleData(*dataPtr);
        } else {
            std::println(std::cerr, "Routing Error: 'data' path expected dataTransaction payload.");
        }
    }}
};

void dispatch(const std::string &cmd, RequestVariant &req, ws_stream* ws) {
    auto it = functionTable.find(cmd);
    if (it != functionTable.end()) {
        if (cmd == "data") {
            if (auto *dataPtr = std::get_if<data::dataTransaction>(&req)) {
                dataPtr->socketContext = ws; // Simply assigns the pointer directly (handles null safely!)
                std::println( "NOTIFY: OP:{} DISPATCHING! " , cmd);

            }
        }
        it->second(req); 
    } else {
        std::println(std::cerr, "Command: '{}' not supported by endpoint routing matrices.", cmd);
    }
}
