#include <web/router.hpp>
#include <web/loginhandler.hpp>
#include <web/datahandler.hpp>
#include <print>       
#include <iostream>    

// 1. Definition of explicit business logic anchors
void handleLogin(loginRequest &req) {
    login(req);
}

void handleData(data::dataTransaction &tx) {
    handleDataSync(tx);
}

// 2. Exact allocation and definition of your lambda-packed functionTable map
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

// 3. Definition of the top-level route validation loop
void dispatch(const std::string &cmd, RequestVariant &req) {
    auto it = functionTable.find(cmd);
    if (it != functionTable.end()) {
        it->second(req); 
    } else {
        std::println(std::cerr, "Command: '{}' not supported by endpoint routing matrices.", cmd);
    }
}
