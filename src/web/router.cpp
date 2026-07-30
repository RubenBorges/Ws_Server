#include <BPY/util.hpp>
#include <web/datahandler.hpp>
#include <web/loginhandler.hpp>
#include <web/router.hpp>
#include <sys/types.h>
#include <cstdint>
#include <iostream>    
#include <print>      
#include <unordered_map> 
#include <functional>    
#include <any>


extern std::vector<std::string> dataLogBuilder;
extern std::vector<std::string> activeServerFilePaths;
extern data::dataTransaction ThisDataTransaction;


namespace router{
// The pattern definition
template<class... Ts> struct overloaded : Ts... { using Ts::operator()...; };
template<class... Ts> overloaded(Ts...) -> overloaded<Ts...>;

int visit() {
    std::variant<loginRequest, data::dataTransaction> reqVariant = loginRequest{};
    std::visit(overloaded {
        [](const loginRequest& req) { 
            std::cout << "Processing a Login\n"; 
        },
        [](const data::dataTransaction& req) { 
            std::cout << "Processing a DataRequest\n"; 
        }
    }, reqVariant);
    
    return 0;
}
}//namespace router

void handleLogin(loginRequest &req) {
    login(req);
}

void handleDataRequest(data::dataTransaction& request, std::filesystem::path _target, std::vector<uint8_t>& _buffer) {
    handleDataSync(request,_target,_buffer); 
}

using LoginFuncPtr = void(*)(loginRequest&);
using DataFuncPtr  = void(*)(data::dataTransaction&, std::filesystem::path, std::vector<uint8_t>&);
using FastFuncVariant = std::variant<LoginFuncPtr, DataFuncPtr>;

void dispatch(const std::string &cmd, RequestVariant &req, std::filesystem::path& _target, std::vector<uint8_t>& _buffer) {
    // 3. Build a static map of raw function pointers wrapped in variants
    static const std::unordered_map<std::string, FastFuncVariant> function_table {
        { "login", LoginFuncPtr([](loginRequest& login_req) { handleLogin(login_req); }) },
        { "data",  DataFuncPtr([](data::dataTransaction& request, std::filesystem::path target_path, std::vector<uint8_t>& buffer) { 
                       handleDataRequest(request, target_path, buffer);})}
    };

    // 4. Validate command existence
    auto it = function_table.find(cmd);
    if (it == function_table.end()) {
        std::println(std::cerr, "Command: '{}' not supported.", cmd);
        return;
    }

    // 5. Unpack and execute directly using raw pointer speeds
    if (cmd == "login") {
        if (auto* login_req = std::get_if<loginRequest>(&req)) {
            auto func = std::get<LoginFuncPtr>(it->second);
            func(*login_req); // Direct function pointer call
        } else {
            std::println(std::cerr, "Error: Payload is not a loginRequest.");
        }
    } 
    else if (cmd == "data") {
        if (auto* data_req = std::get_if<data::dataTransaction>(&req)) {
            auto func = std::get<DataFuncPtr>(it->second);
            func(*data_req, _target, _buffer); // Direct function pointer call
        } else {
            std::println(std::cerr, "Error: Payload is not a dataTransaction.");
        }
    }
}
/*
void dispatch(const std::string &cmd, RequestVariant &req, std::filesystem::path& _target, std::vector<uint8_t>& _buffer) {
    // 1. Declare the table static so it is built only once, saving CPU cycles on subsequent calls
    static const std::unordered_map<std::string, std::any> function_table {
        {
            "login", 
            std::function<void(loginRequest&)>(
                [](loginRequest& login_req) { 
                    handleLogin(login_req); 
                }
            )
        },
        {
            "data", 
            std::function<void(data::dataTransaction&, std::filesystem::path, std::vector<uint8_t>&)>(
                [](data::dataTransaction& request, std::filesystem::path target_path, std::vector<uint8_t>& buffer) { 
                   handleDataRequest(request, target_path, buffer);
                }
            )
        }
    };
    auto it = function_table.find(cmd);
    if (it == function_table.end()) {
        std::println(std::cerr, "Command: '{}' not supported.", cmd);
        return;
    }
    if (cmd == "login") {
        if (auto* login_req = std::get_if<loginRequest>(&req)) {
            auto func = std::any_cast<std::function<void(loginRequest&)>>(it->second);
            func(*login_req);
        } else {
            std::println(std::cerr, "Error: Command 'login' requested, but payload is not a loginRequest.");
        }
    } 
    else if (cmd == "data") {
        if (auto* data_req = std::get_if<data::dataTransaction>(&req)) {
            auto func = std::any_cast<std::function<void(data::dataTransaction&, std::filesystem::path, std::vector<uint8_t>&)>>(it->second);
            func(*data_req, _target, _buffer);
        } else {
            std::println(std::cerr, "Error: Command 'data' requested, but payload is not a dataTransaction.");
        }
    }
    // --- Networking/IO Logic Cleanup ---
    // auto io = bpy::utility::make_io_context();
    // ws_stream ws{std::move(io)}; 
}*/