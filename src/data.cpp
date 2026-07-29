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
    loginRequest myLogin{LogCmd::LOGIN, loginCredentials{"Admin", "PASSWD"},uuid_T{}};
    RequestVariant loginPayload = myLogin; 
    p2p::web::ClientOptions clientOptions {"127.0.0.1",4557};
    data::WebSocket WebSock {clientOptions};
    WebSock.connect();
    p2p::web::AsyncEchoClient aClient(WebSock.IO());   
    p2p::web::ServerOptions serverOptions{clientOptions.port};
    p2p::web::AsyncEchoServer aServer(WebSock.IO(),serverOptions);
    
    // ---- Test Path 2: Execute a Data Transfer Flow ----
    using namespace data;
    
    dispatch("login",loginPayload,WebSock.ws);
    data::dataTransaction ThisDataTransaction("/home/boopy/Pictures/boop.jpg", 1024,"home/boopy/dev/Projects/ws_server/out",&WebSock.ws,OP::CP);
    RequestVariant reqVariant(ThisDataTransaction);
    dispatch("data",reqVariant,*ThisDataTransaction.ws); 
    return 0;
}