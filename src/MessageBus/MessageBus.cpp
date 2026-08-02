#include "stdinc.hpp"
#include <iostream>
#include <expected>
#include <string>

struct Message {
  std::string content;
};

struct Processor{};

class InterfacePolicy {
public: 
  virtual ~InterfacePolicy() = default;
  virtual void interface(Message message, std::expected<int, std::string>) = 0;
};

/*-------------------------------\\
|| On System Interface Policies  ||
\\-------------------------------*/

//Development and Debugging
struct StdIOInterfacer  : public InterfacePolicy {
  void interface(Message message, std::expected<int, std::string>) override {
      std::cout << "StdIOInterfacer received message: " << message.content << std::endl;
      return;
  };
};

// Inter-Process Communication (IPC) Policies
struct PipedInterfacer  : public InterfacePolicy{
  void interface(Message message, std::expected<int, std::string>) override {
      std::cout << "PipedInterfacer received message: " << message.content << std::endl;
      return;
  };
};
struct AF_UnixInterfacer: public InterfacePolicy{
  void interface(Message message, std::expected<int, std::string>) override {
      std::cout << "AF_UnixInterfacer received message: " << message.content << std::endl;
      return;
  };
};


/*----------------------------\\
|| Network Interface Policies ||
\\----------------------------*/

// Off System Interfacer Policies
struct WebSockInterfacer: public InterfacePolicy {
  void interface(Message message, std::expected<int, std::string>) override {
      std::cout << "WebSockInterfacer received message: " << message.content << std::endl;
      return;
  };
};
struct WebHookInterfacer: public InterfacePolicy{
  void interface(Message message, std::expected<int, std::string>) override {
      std::cout << "WebHookInterfacer received message: " << message.content << std::endl;
      return;
  };
};

/*===================================\\
|| Host Class:                       ||
|| Message Protocol Service Template ||
\\===================================*/
  //Uses Composition to allow for flexible interfacing with various communication protocols and mediums, 
  // enabling seamless message handling across different platforms and environments.

template <typename InterfacePolicy, typename MessageType>
class MessageProtocolService {
public:
  void sendMessage(const MessageType& message) {
      InterfacePolicy policy;
      std::expected<int, std::string> result; // Placeholder for actual result
      policy.interface(message, result);
  }
};

template <typename Policy>
void dispatchMessage(const Message& message, const std::string& label) {
  std::cout << "\n-- [" << label << "] --" << std::endl;
  MessageProtocolService<Policy, Message> service;
  service.sendMessage(message);
}

int MessageService() {
  std::cout << "Interface Policy Demo" << std::endl;

  Message baseMessage{"Hello, Interface Policy!"};
  dispatchMessage<StdIOInterfacer>(baseMessage, "StdIOInterfacer");
  dispatchMessage<PipedInterfacer>({"This message travels over a pipe."}, "PipedInterfacer");
  dispatchMessage<AF_UnixInterfacer>({"AF_UNIX message payload."}, "AF_UnixInterfacer");
  dispatchMessage<WebSockInterfacer>({"WebSocket event payload."}, "WebSockInterfacer");
  dispatchMessage<WebHookInterfacer>({"Webhook callback received."}, "WebHookInterfacer");

  std::cout << "\nAll example dispatches complete." << std::endl;
  return 0;
}
