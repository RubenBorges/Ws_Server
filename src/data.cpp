#include "web/datahandler.hpp"
#include <BPY/util.hpp>
#include <boost/asio.hpp>
#include <boost/asio/buffer.hpp>
#include <boost/asio/ip/tcp.hpp>
#include <boost/beast/websocket.hpp>
#include <boost/random/random_device.hpp>
#include <boost/uuid/uuid.hpp>
#include <boost/uuid/uuid_generators.hpp>
#include <boost/uuid/uuid_io.hpp>
#include <web/client.hpp>
#include <web/server.hpp>
#include <boost/beast/core.hpp>
#include <boost/asio/connect.hpp>
#include <boost/asio/io_context.hpp>
#include <boost/beast/websocket/stream.hpp>
#include <jwt-cpp/jwt.h>

#include <variant>
#include <web/router.hpp>
#include <web/log.hpp>

// Custom definitions mapping network socket namespaces cleanly
namespace asio = boost::asio;
using tcp = asio::ip::tcp;
using ws_stream = boost::beast::websocket::stream<tcp::socket>;
using RequestVariant = std::variant<loginRequest, data::dataTransaction>;

using LoginFuncPtr = void(*)(loginRequest&);
using DataFuncPtr  = void(*)(data::dataTransaction&,data::DataProcessor&);
using FastFuncVariant = std::variant<LoginFuncPtr, DataFuncPtr>;

// ==========================================
// 3. Main Test Suite
// ==========================================
int main() {

    // Setup Test payloads
    loginRequest myLogin{LogCmd::LOGIN, loginCredentials{"Admin", "PASSWD"}, uuid_T{}};
    p2p::web::ClientOptions clientOptions {"127.0.0.1", 4557};
    data::WebSocket WebSock {clientOptions};
    
    //p2p::web::AsyncEchoClient aClient(WebSock.IO());   
    //p2p::web::ServerOptions serverOptions{clientOptions.port};
    //p2p::web::AsyncEchoServer aServer(WebSock.IO(), serverOptions);
    

    RequestVariant loginPayload = myLogin; 
   

    using namespace data;
    dataTransaction dataReq{OP::CP, WebSock.ws};
    RequestVariant dataPayload = dataReq;
    DataProcessor dat {"targ.ta", {0}, "dest"};
    
    // router::visit(loginPayload, dat); // CRITICAL: This is how you execute the login pipeline natively
    // router::visit(dataPayload, dat); // CRITICAL: This is how you execute the data pipeline natively
//router::visit(loginFnPtr, loginPayload, dat);
//router::visit(dataFnPtr, dataPayload, dat);
    // REMOVED: dispatch() and manual FastFuncVariant pointer wrappers are wiped away!
    return 0;
}