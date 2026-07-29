#include <BPY/util.hpp>
#include <web/datahandler.hpp>
#include <web/router.hpp>
#include <print>      
#include <variant>
#include <iostream>    
#include <unordered_map> 
#include <functional>    
#include <boost/asio/ip/tcp.hpp>
#include <boost/beast/websocket/stream.hpp>
#include <boost/asio/io_context.hpp>
#include <iostream>
#include <variant>

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

void handleDataRequest(data::dataTransaction& request) {
    handleDataSync(request); 
}

const std::unordered_map<std::string, std::function<void(RequestVariant*, ws_stream&)>> functionTable = {
    { "login", [](RequestVariant* req, ws_stream& ws) { 
    if (auto* loginPtr = std::get_if<loginRequest>(req); loginPtr != nullptr) {
            handleLogin(*loginPtr);
        } else {std::clog << "Routing Error: Expected loginRequest payload.\n";}}},

    { "data", [](RequestVariant* req, ws_stream& ws) { 
    if (auto* dPtr = std::get_if<data::dataTransaction>(req); dPtr != nullptr) {
            handleDataRequest(*dPtr);}
    else {std::clog << "Routing Error: Expected dataTransaction payload.\n";}}}
};

void dispatch(const std::string &cmd, RequestVariant &req, ws_stream& ws) {
    auto it = functionTable.find(cmd);
    if (it == functionTable.end()) {
        std::println(std::cerr, "Command: '{}' not supported.", cmd);
        return;
    }
    
    // 4. Corrected execution: Pass the pointer to the variant (&req) and use the iterator 'it'
    it->second(&req, ws);

    // 5. Cleaned up trailing example code to make it valid syntax
    auto io = bpy::utility::make_io_context(); // Assuming factory returns a shared_ptr or io_context
    // ws_stream wss{std::move(io)}; // Removed: Streams cannot be constructed directly from a factory object like this
}
