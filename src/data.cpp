#include <BPY/util.hpp>
#include <boost/asio.hpp>
#include <boost/asio/buffer.hpp>
#include <boost/asio/ip/tcp.hpp>
#include <boost/beast/websocket.hpp>
#include <boost/random/random_device.hpp>
#include <boost/uuid/uuid.hpp>
#include <boost/uuid/uuid_generators.hpp>
#include <boost/uuid/uuid_io.hpp>
#include <cstdint>
#include <memory>
#include <web/client.hpp>
#include <web/server.hpp>
#include <boost/beast/core.hpp>
#include <boost/asio/connect.hpp>
#include <boost/asio/io_context.hpp>
#include <boost/beast/websocket/stream.hpp>
#include <jwt-cpp/jwt.h>
#include <string>
#include <variant>
#include <web/datahandler.hpp>
#include <web/loginhandler.hpp>
#include <web/router.hpp>

// Custom definitions mapping network socket namespaces cleanly
namespace asio = boost::asio;
using tcp = asio::ip::tcp;
using ws_stream = boost::beast::websocket::stream<tcp::socket>;
using RequestVariant = std::variant<loginRequest, data::dataTransaction>;

int main() {
    std::vector<std::string> dataLogBuilder; 
    std::vector<std::string> logBuilder;
    std::vector<std::vector<std::string>*> Loglist;
    Loglist.push_back(&logBuilder);
    Loglist.push_back(&dataLogBuilder);


    loginRequest myLogin{LogCmd::LOGIN, loginCredentials{"Admin", "PASSWD"},uuid_T{}};
    p2p::web::ClientOptions clientOptions {"127.0.0.1",4557};
    data::WebSocket WebSock {clientOptions};
    // WebSock.connect();
     p2p::web::AsyncEchoClient aClient(WebSock.IO());   
     p2p::web::ServerOptions serverOptions{clientOptions.port};
     p2p::web::AsyncEchoServer aServer(WebSock.IO(),serverOptions);
    
    // ---- Test Path 2: Execute a Data Transfer Flow ----
    using namespace data;
    
    DataProcessor procesor {"Target"};
    RequestVariant loginPayload = myLogin; 
    dispatch("login",loginPayload,procesor.target,procesor.buffer);
    data::dataTransaction dataReq{data::OP::CP, WebSock.ws};
    RequestVariant dataPayload = dataReq;
    dispatch("data",dataPayload, procesor.target,procesor.buffer); 
    return 0;
}