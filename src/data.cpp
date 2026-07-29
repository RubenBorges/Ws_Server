
#include "dir_crawler.hpp"
#include <BPY/util.hpp>
#include <algorithm>
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
    loginRequest myLogin{LogCmd::LOGIN, {"Admin", "PASSWD"},{}};

    // Wrap the request inside the polymorphic variant
    RequestVariant loginPayload = myLogin; 
    dispatch("login", loginPayload);

    // Extract the mutated state back out if needed
    myLogin = std::get<loginRequest>(loginPayload);


    // ---- Test Path 2: Execute a Data Transfer Flow ----
    data::dataTransaction myDataTx;
    myDataTx.cmd = data::OP::RX;
    myDataTx.targetPath = "/home/boopy/Pictures/out";

    // Wrap and dispatch seamlessly using the identical signature entry pointer
    RequestVariant dataPayload = myDataTx;
    dispatch("data", dataPayload);
    
    return 0;
}
