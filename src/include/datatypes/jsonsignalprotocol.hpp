#pragma once
#include <cstdint>
#include <string>

struct JsonSignalProtocol {
  enum class SIG: int { REGISTER= 1, PEERLIST = 2, CONNECT = 3, OFFER = 4, ANSWER = 5 } signal;
  std::string type; // "offer", "answer", "ice-candidate"
  std::string sdp;  // For "offer" and "answer"

  // client registers
  void register_peer(const std::string &peer_id) {
    signal = SIG::REGISTER;
    type = "register";
    sdp = peer_id; // using sdp field to store peer_id for simplicity
  }

  // client requests connected peers
  void peer_list() {
    signal = SIG::PEERLIST;
    type = "peer_list";
    sdp = ""; // no additional data needed
  }
  // client wants to connect to another peer
  void connect(const std::string &target) {
    signal = SIG::CONNECT;
    type = "connect";
    sdp = target; // using sdp field to store target peer_id for simplicity
  }
  // server forwards connection info to bob
  void offer(const std::string &from, const std::string &addr, uint16_t port) {
    signal = SIG::OFFER;
    type = "offer";
    sdp = from + "," + addr + "," + std::to_string(port); // "alice,1.2.3.4,9000"
  }

  // bob replies, server forwards to alice
  void answer(const std::string &from, const std::string &addr, uint16_t port) {
    signal = SIG::ANSWER;
    type = "answer";
    sdp = from + "," + addr + "," + std::to_string(port); // "bob,5.6.7.8,9001"
  }
};
/* Example JSON messages for signaling:
{"type":"register", "peer_id":"alice"}
{"type":"peer_list"}
{"type":"connect", "target":"bob"}
{"type":"offer", "from":"alice", "addr":"1.2.3.4", "port":9000}
{"type":"answer", "from":"bob", "addr":"5.6.7.8", "port":9001}
*/

