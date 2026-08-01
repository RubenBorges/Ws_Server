#include "web/loginhandler.hpp"
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
#include <string>
#include <vector>
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
  dispatch(RequestType::LOGIN, loginPayload);
   
  std::filesystem::path targt {"/home/boopy/dev/Projects/ws_server/out/out"};
  std::filesystem::path dest {"/home/boopy/dev/Projects/ws_server/out/copy"};
  std::vector<uint8_t> buffer;
    //DataProcessor dat {"targ.ta", {0}, "dest"};
    data::dataTransaction dataReq{data::OP::CP, WebSock.ws, data::DataProcessor{targt, &buffer, dest}};
    RequestVariant dataPayload = dataReq;
      dispatch(RequestType::DATA, dataPayload);
    

    return 0;
}