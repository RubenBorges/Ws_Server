#include <web/server.hpp>
#include <web/client.hpp>

#include <boost/asio/co_spawn.hpp>
#include <boost/asio/executor_work_guard.hpp>
#include <boost/asio/io_context.hpp>
#include <boost/asio/use_future.hpp>

#include <thread>

//-----------------------------------------------
void ServerConnect_AsyncCoroutine() {
    boost::asio::io_context server_io(1);
    auto server = p2p::web::make_server(
        server_io, p2p::web::ServerOptions{5555, "/shutdown"});
    server->start();
    server_io.run();
}

void ClientConnect_AsyncCoroutine() {
    namespace asio = boost::asio;
    asio::io_context client_io(1);
    auto client_work = asio::make_work_guard(client_io);
    std::thread io_thread([&client_io]() {
        client_io.run();
    });

    p2p::web::AsyncEchoClient client(client_io);
    auto connect_future = asio::co_spawn(
        client_io,
        client.connect(p2p::web::ClientOptions{"127.0.0.1", 5555}),
        asio::use_future);
    connect_future.get();

    auto reply_future = asio::co_spawn(
        client_io,
        client.send_and_receive("hello"),
        asio::use_future);
    (void)reply_future.get();

    client.close();
    client_work.reset();
    client_io.stop();
    io_thread.join();
}

/* 1. Why the Initial Server Contact? (The "Signaling" Phase)
Two peers cannot simply "find" each other on the open internet because they are usually hidden behind firewalls and routers (NAT). The server acts as a matchmaker or "signaling server" to: 

    -> Exchange Identity: Help Peer A and Peer B find each other's public IP addresses.

    -> Coordinate the Handshake: Relay the initial "Offer" and "Answer" messages needed to negotiate a connection.
    
    -> Traverse Firewalls: Use protocols like STUN to figure out your public-facing IP so the other peer knows where to send data. 
*/

/*2. The "Forwarding" to P2P
Once the server has helped the two peers exchange their connection details (metadata), it steps out of the way. The peers then attempt to establish a direct, encrypted data pipe between them. 

    -> If it works: You have a true direct P2P connection where data (like video or files) never touches the server again.
    
    ->    If it fails: If a firewall is too restrictive, the system might fall back to a TURN server, 
    which acts as a relay—essentially "forwarding" the traffic through a middleman to ensure it still reaches the destination. */

/*
3. Common Use Cases
You will see this "Server-to-P2P" pattern in:

    -> Video/Audio Calls: Apps like Discord, Zoom, or WhatsApp use servers to ring the other person, but the actual voice data often goes P2P.
    
    -> File Sharing: Services like BitTorrent or WebTorrent use trackers (servers) to find peers, but the files are sent directly between users.
    
    -> Real-time Gaming: A central server matches players into a lobby, then hands off the actual gameplay updates to a P2P mesh to reduce lag. 
*/