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
#include <memory>
#include <filesystem>
#include <jwt-cpp/jwt.h>
#include <string>
#include <variant>
#include <web/datahandler.hpp>
#include <web/loginhandler.hpp>
#include <web/router.hpp>
// Custom definitions mapping network socket namespaces cleanly
using RequestVariant = std::variant<loginRequest, data::dataTransaction>;


namespace asio = boost::asio;
namespace beast = boost::beast;
namespace websocket = beast::websocket;
using tcp = asio::ip::tcp;
using ws_stream = boost::beast::websocket::stream<tcp::socket>;
int main() {

    // ---- Test Path 1: Execute a Login Flow ----
    // FIXED: Cleaned up structure list initializer elements to match loginRequest parameters
    loginRequest myLogin{LogCmd::LOGIN, loginCredentials{"Admin", "PASSWD"},uuid_T{}};

    // Wrap the request inside the polymorphic variant
    RequestVariant loginPayload = myLogin; 
    // Invoke using a safe cast layer for local simulation
    //dispatch("login", loginPayload, ws_stream{nullptr});    
    
    // Extract the mutated state back out if needed
    myLogin = std::get<loginRequest>(loginPayload);
    
    boost::system::error_code ec;
    p2p::web::ClientOptions clientOptions {"127.0.0.1",4557};
    boost::asio::io_context io(1);
    tcp::socket socket{io};
    tcp::resolver resolv{io};
    auto res = resolv.resolve(clientOptions.host,std::to_string(clientOptions.port));
    asio::connect(socket, res.begin(), res.end());
    websocket::stream<tcp::socket> ws{std::move(socket)};
    ws.handshake(clientOptions.host, "/");
    ws.binary(true);
    std::string message;
    ws.write(asio::buffer(message));
    
    p2p::web::AsyncEchoClient aClient(io);   
    
    p2p::web::ServerOptions serverOptions{4577};
    
    p2p::web::AsyncEchoServer aServer(io,serverOptions);
    
    // ---- Test Path 2: Execute a Data Transfer Flow ----
    using namespace data;

    data::dataTransaction ThisDataTransaction("/home/boopy/Pictures/boop.jpg", 1024,"home/boopy/dev/Projects/ws_server/out", ws_stream(socket),OP::CP);
    
    //RequestVariant* lst{{static_cast<RequestVariant>(myLogin)},{static_cast<RequestVariant>(ThisDataTransaction)}};
    RequestVariant myVariant;
    myVariant.emplace<data::dataTransaction>(ThisDataTransaction);
    // and the last parameter must resolve to a reference 'ws_stream&' matching your router header signature.
    dispatch("data",ThisDataTransaction); 
    return 0;
}


/*
// 1. Base struct MUST be polymorphic for dynamic_cast to compile
struct Req {
    std::string id;
    Req(std::string str):id(str){};
    virtual ~Req() = default; // Added virtual destructor
};

struct loginReq : public Req {
    std::string name;
    loginReq(std::string str) : Req{"1333"}, name(str) {}
};

struct dataTransaction : public Req {
    std::vector<uint8_t> buffer;
    dataTransaction(int size) : Req{"1234"} { buffer.reserve(size); }
};
   loginReq log{"sudhuas"}; 
    dataTransaction dat{50};

    std::vector<Req*> reqPt;
    reqPt.push_back(&log);
    reqPt.push_back(&dat);

    // Get the first element (loginReq)
    Req* basePtr = reqPt.front();
    
    // This now compiles and works perfectly!
    loginReq* ks = dynamic_cast<loginReq*>(basePtr);

    // Always verify the cast succeeded
    if (ks != nullptr) {
        std::cout << "Success! Found loginReq with name: " << ks->name << "\n";
    } else {
        std::cout << "Cast failed.\n";
    }*/