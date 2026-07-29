#include <BPY/util.hpp>
#include <boost/asio/buffer.hpp>
#include <boost/asio/ip/tcp.hpp>
#include <boost/beast/websocket.hpp>
#include <boost/random/random_device.hpp>
#include <boost/uuid/uuid.hpp>
#include <boost/uuid/uuid_generators.hpp>
#include <boost/uuid/uuid_io.hpp>

#include <filesystem>
#include <jwt-cpp/jwt.h>
#include <string>

#include <web/datahandler.hpp>
#include <web/loginhandler.hpp>
#include <web/router.hpp>

int main() {
    // ---- Test Path 1: Execute a Login Flow ----
    // FIXED: Cleaned up structure list initializer elements to match loginRequest parameters
    loginRequest myLogin{LogCmd::LOGIN, loginCredentials{"Admin", "PASSWD"}};

    // Wrap the request inside the polymorphic variant
    RequestVariant loginPayload = myLogin; 
    
    // Invoke using a safe cast layer for local simulation
    dispatch("login", loginPayload);    
    
    // Extract the mutated state back out if needed
    myLogin = std::get<loginRequest>(loginPayload);


    // ---- Test Path 2: Execute a Data Transfer Flow ----
    data::dataTransaction myDataTx;
    // FIXED: Changed 'CP' to 'RX' (Read operation) because CP does not exist in your OP enum layout
    myDataTx.cmd = data::OP::CP; 
    myDataTx.targetPath = "/home/boopy/Pictures/out";
    myDataTx.outputPath = "/home/boopy/Projects/ws_server";

    // Wrap and dispatch seamlessly using the identical signature entry pointer
    RequestVariant dataPayload = myDataTx;

    // FIXED: The first parameter must be the routing key string ("data"), 
    // and the last parameter must resolve to a reference 'ws_stream&' matching your router header signature.
    dispatch("data", dataPayload);    
    return 0;
}
