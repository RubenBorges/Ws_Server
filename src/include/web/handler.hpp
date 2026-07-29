#pragma once 

#include <string>
#include <unordered_map>
#include <functional> 
#include <print>       
#include <iostream>    
#include <variant>     // Required for type-safe polymorphic structures
#include <boost/uuid/uuid_io.hpp> 

#include "loginhandler.hpp" // Ensure correct header path mapping
#include "datahandler.hpp"  // Ensure correct header path mapping

// 1. Create a type-safe wrapper variant containing all supported system request payloads
using RequestVariant = std::variant<loginRequest, data::dataTransaction>;

// 2. Business logic forwarding anchors mapping parameters cleanly
inline void handleLogin(loginRequest &req) {
    login(req);
}

inline void handleData(data::dataTransaction &tx) {
    handleDataSync(tx);
}

// 3. Polymorphic router table capable of matching different argument signatures
inline const std::unordered_map<std::string, std::function<void(RequestVariant &)>> functionTable = {
    { "login", [](RequestVariant &req) {
        // std::get_if validates the type variant matches at runtime
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

// 4. Dispatch entry router routing any structural variant type
inline void dispatch(const std::string &cmd, RequestVariant &req) {
    auto it = functionTable.find(cmd);
    if (it != functionTable.end()) {
        it->second(req); 
    } else {
        std::println(std::cerr, "Command: '{}' not supported by endpoint routing matrices.", cmd);
    }
}
