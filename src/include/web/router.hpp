#pragma once 

#include <string>
#include <unordered_map>
#include <functional> 
#include <variant>     
#include <boost/uuid/uuid_io.hpp> 

#include "loginhandler.hpp" 
#include "datahandler.hpp"  

// 1. Core type-safe polymorphic variant alias
using RequestVariant = std::variant<loginRequest, data::dataTransaction>;

// 2. Business logic forward declarations
void handleLogin(loginRequest &req);
void handleData(data::dataTransaction &tx);

// 3. Declare the global routing matrix table as extern
extern const std::unordered_map<std::string, std::function<void(RequestVariant &)>> functionTable;

// 4. Central entry router forwarding function
void dispatch(const std::string &cmd, RequestVariant &req);
