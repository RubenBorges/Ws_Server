#include <BPY/util.hpp>
#include <web/datahandler.hpp>
#include <web/loginhandler.hpp>
#include <web/router.hpp>
#include <sys/types.h>
#include <cstdint>
#include <iostream>    
#include <print>      
#include <unordered_map> 


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

void handleDataRequest(data::dataTransaction& request,const  std::filesystem::path* _target, std::vector<uint8_t>& _buffer) {
    handleDataSync(request,_target,_buffer); 
}

using LoginFuncPtr = void(*)(loginRequest&);
using DataFuncPtr  = void(*)(data::dataTransaction&, const std::filesystem::path*, std::vector<uint8_t>&);
using FastFuncVariant = std::variant<LoginFuncPtr, DataFuncPtr>;

void dispatch(const std::string &cmd, RequestVariant &req, const std::filesystem::path* _target, std::vector<uint8_t>& _buffer) {
    static const std::unordered_map<std::string, FastFuncVariant> function_table {
        { "login", LoginFuncPtr([](loginRequest& login_req) { handleLogin(login_req);})},
        { "data",  DataFuncPtr([](data::dataTransaction& request,const std::filesystem::path* target_path, std::vector<uint8_t>& buffer) { 
                       handleDataRequest(request, target_path, buffer);})}
    };

    auto it = function_table.find(cmd);
    if (it == function_table.end()) {
        std::println(std::cerr, "Command: '{}' not supported.", cmd);
        return;
    }

    if (cmd == "login") {
        if (auto* login_req = std::get_if<loginRequest>(&req)) {
            auto func = std::get<LoginFuncPtr>(it->second);
            func(*login_req); }
        else{std::println(std::cerr, "Error: Payload is not a loginRequest.");}
    } 
    else if (cmd == "data") {
        if (auto* data_req = std::get_if<data::dataTransaction>(&req)) {
            auto func = std::get<DataFuncPtr>(it->second);
            func(*data_req, _target, _buffer);} 
        else {std::println(std::cerr, "Error: Payload is not a dataTransaction.");}
    }
}