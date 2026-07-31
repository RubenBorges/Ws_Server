#include <BPY/util.hpp>
#include <optional>
#include <web/datahandler.hpp>
#include <web/loginhandler.hpp>
#include <web/router.hpp>
#include <web/log.hpp>
#include <sys/types.h>
#include <iostream>    
#include <print>      
#include <unordered_map> 

void handleLogin(loginRequest &req) {    
    login(req);
    logBuilder.push_back(std::format("T:{:%Y-%m-%d %H:%M:%S} -- {}",std::chrono::system_clock::now(), (req.status==LogResult::SUCCESS)? "NOTIFY:Login Request Handler Success": "ERROR:Login Request Handler Failed"));
}

void handleDataRequest(data::dataTransaction& request, data::DataProcessor& data) {
    handleDataSync(request, data); 
    dataLogBuilder.push_back(std::format("T:{:%Y-%m-%d %H:%M:%S} -- {}",std::chrono::system_clock::now(), (request.status==data::Result::SUCCESS)? "NOTIFY:DataRequest Handler Success": "ERROR:DataRequest Handler Failed"));
}

using LoginFuncPtr = void(*)(loginRequest&);
using DataFuncPtr  = void(*)(data::dataTransaction&,data::DataProcessor&);
using FastFuncVariant = std::variant<LoginFuncPtr, DataFuncPtr>;

void dispatch(const std::string& cmd, RequestVariant &req, data::DataProcessor& data) {
    static const std::unordered_map<std::string, FastFuncVariant> function_table {
        { "login", LoginFuncPtr([](loginRequest& login_req) { handleLogin(login_req);})},
        { "data",  DataFuncPtr([](data::dataTransaction& request, data::DataProcessor& dat) { 
                       handleDataRequest(request, dat);})}
    };

    auto it = function_table.find(cmd);
    if (it == function_table.end()) {
        std::println(std::cerr, "Command: '{}' not supported.", cmd);
        return;
    }
    if (cmd == "login") {
        if (auto login_req = std::get_if<loginRequest>(&req)) {
            auto func = std::get<LoginFuncPtr>(it->second);
            logBuilder.push_back(std::format("T:{:%Y-%m-%d %H:%M:%S} -- Notify:Dispatching Login Handler For {}",std::chrono::system_clock::now(),login_req->logCredentials.username));
            func(*login_req); }
        else{logBuilder.push_back(std::format("T:{:%Y-%m-%d %H:%M:%S} -- Error: Payload is not a loginRequest.",std::chrono::system_clock::now()));}
    } 
    else if (cmd == "data") {
        if (auto data_req = std::get_if<data::dataTransaction>(&req)) {
            auto func = std::get<DataFuncPtr>(it->second);
            dataLogBuilder.push_back(std::format("T:{:%Y-%m-%d %H:%M:%S} -- Notify:Dispatching DataRequest Handler",std::chrono::system_clock::now()));
            func(*data_req, data);} 
        else{dataLogBuilder.push_back(std::format("T:{:%Y-%m-%d %H:%M:%S} -- Error: Payload is not a DataRequest.",std::chrono::system_clock::now()));}
    }
}

/*
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
*/