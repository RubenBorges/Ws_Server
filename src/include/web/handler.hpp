#pragma once // Crucial to prevent duplicate include compilation errors

#include <string>
#include <unordered_map>
#include <functional> // Required for std::function
#include <print>       // Required for std::println
#include <iostream>    // Required for std::cerr
#include <boost/uuid/uuid_io.hpp> // Required for boost::uuids::to_string
#include "login.hpp"
// 1. Handles a login command wrapper forwarding requests directly to business logic layers
inline void handleLogin(loginRequest &req) {login(req);}

// 2. Handles a data-processing command placeholder for the server-side dispatcher
inline void handleData(loginRequest &req) {std::println("Processing data payload requests for UUID: {}",boost::uuids::to_string(req.uuid));}

// 3. Modernized type-safe map using std::function closures
inline const std::unordered_map<std::string, std::function<void(loginRequest &)>> functionTable = {{"login", handleLogin}, {"data", handleData}};

// 4. Dispatch entry router routing transactional request state payloads
inline void dispatch(const std::string &cmd, loginRequest &req) {
    auto it = functionTable.find(cmd);
    if (it != functionTable.end()) {
        it->second(req); // Invoke type-safe dynamic execution paths cleanly
    } else {
        std::println(std::cerr, "Command: '{}' not supported by endpoint routing matrices.", cmd);
    }
}
