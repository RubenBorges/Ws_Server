#include <unordered_map>
#include <string>
#include <functional>
#include <web/login.hpp>
// MODERNIZED FIX: Replaced raw function pointers with decoupled std::function signatures passing requests by reference
static const std::unordered_map<std::string, std::function<void(loginRequest&)>> functionTable = {
    {"login", handleLogin},
    {"data", handleData}
};

// Looks up a command name in the function table and invokes the matching handler with input payload state references
void dispatch(const std::string& cmd, loginRequest& req) {
    // Safe lookup method instead of using operator[] which introduces garbage collection insertions
    auto it = functionTable.find(cmd);
    
    if (it != functionTable.end()) {
        it->second(req); // Invoke type-safe dynamic payload functor execution loop paths
    } else {
        std::println(std::cerr, "Command: '{}' not supported by endpoint routing matrices.", cmd);
    }
}
