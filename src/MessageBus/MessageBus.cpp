#include <iostream>
#include <fstream>
#include <expected>
#include <string>
#include <type_traits>
#include <utility>
#include <iostream>
#include <string>
#include <expected>
/*-------------------------------\\
|| 1. Endpoint Value Structures  ||
\\-------------------------------*/
struct ConsoleEndpoint {
    std::string stream_name = "stdout";
};

struct PipeEndpoint {
    std::string pipe_path;
};

struct AF_UnixEndpoint {
    std::string socket_path;
};

struct SocketEndpoint {
    std::string ip_address;
    int port;
};

struct WebHookEndpoint {
    std::string url;
    std::string auth_token;
};

struct FileStreamEndpoint {
    std::ostream& output_stream;
};

/*-------------------------------\\
|| 2. Generic Message Template   ||
\\-------------------------------*/
template <typename EndpointType, typename T>
struct Message {
    T content;
    EndpointType destination;
};

// Automated template inference factory
template <typename EndpointType, typename T>
auto make_message(T&& content, EndpointType&& destination) 
    -> Message<std::decay_t<EndpointType>, std::decay_t<T>> 
{
    return { std::forward<T>(content), std::forward<EndpointType>(destination) };
}

/*-------------------------------\\
|| 3. Structural Policy Tags     ||
\\-------------------------------*/
struct StdIOTag {};
struct PipedTag {};
struct AF_UnixTag {};
struct WebSockTag {};
struct WebHookTag {};
struct FileStreamTag {};

/*-------------------------------\\
|| 4. Finalized Policy Classes   ||
\\-------------------------------*/
// Updated to mirror the new template parameter order: Message<Endpoint, T>
struct StdIOInterfacer {
    template <typename T>
    static void interface(const Message<ConsoleEndpoint, T>& msg, const std::expected<int, std::string>&) {
        std::cout << "[StdIO to " << msg.destination.stream_name << "]: " << msg.content << std::endl;
    }
};

struct PipedInterfacer {
    template <typename T>
    static void interface(const Message<PipeEndpoint, T>& msg, const std::expected<int, std::string>&) {
        std::cout << "[Pipe via " << msg.destination.pipe_path << "]: " << msg.content << std::endl;
    }
};

struct AF_UnixInterfacer {
    template <typename T>
    static void interface(const Message<AF_UnixEndpoint, T>& msg, const std::expected<int, std::string>&) {
        std::cout << "[AF_Unix socket at " << msg.destination.socket_path << "]: " << msg.content << std::endl;
    }
};

struct WebSockInterfacer {
    template <typename T>
    static void interface(const Message<SocketEndpoint, T>& msg, const std::expected<int, std::string>&) {
        std::cout << "[WebSocket to " << msg.destination.ip_address << ":" << msg.destination.port << "]: " << msg.content << std::endl;
    }
};

struct WebHookInterfacer {
    template <typename T>
    static void interface(const Message<WebHookEndpoint, T>& msg, const std::expected<int, std::string>&) {
        std::cout << "[WebHook POST to " << msg.destination.url << " (Token: " << msg.destination.auth_token << ")]: " << msg.content << std::endl;
    }
};

struct FileStreamInterfacer {
    template <typename T>
    static void interface(const Message<FileStreamEndpoint, T>& msg, const std::expected<int, std::string>&) {
        msg.destination.output_stream << "[Log File Entry]: " << msg.content << std::endl;
        std::cout << "[FileStreamInterfacer] Successfully wrote payload to specified file stream." << std::endl;
    }
};

/*-------------------------------\\
|| 5. Policy Traits Selector     ||
\\-------------------------------*/
template <typename InterfaceTag>
struct InterfacePolicySelector;

template <> struct InterfacePolicySelector<StdIOTag>        { using type = StdIOInterfacer; };
template <> struct InterfacePolicySelector<PipedTag>         { using type = PipedInterfacer; };
template <> struct InterfacePolicySelector<AF_UnixTag>       { using type = AF_UnixInterfacer; };
template <> struct InterfacePolicySelector<WebSockTag>       { using type = WebSockInterfacer; };
template <> struct InterfacePolicySelector<WebHookTag>       { using type = WebHookInterfacer; };
template <> struct InterfacePolicySelector<FileStreamTag>    { using type = FileStreamInterfacer; };

/*===================================\\
|| 6. Host Class                     ||
\\===================================*/
template <typename InterfaceTag, typename MessageType>
class MessageProtocolService {
    using Policy = typename InterfacePolicySelector<InterfaceTag>::type;

public:
    void sendMessage(const MessageType& message) {
        std::expected<int, std::string> result; 
        Policy::interface(message, result); 
    }
};

/*-------------------------------\\
|| 7. Execution Demo             ||
\\-------------------------------*/
int SampleUsage() {
    std::cout << "Complete Design with Automated Type Selection\n" << std::endl;

    // 1. Automatic Type Evaluation via Factory & decltype
    auto io_msg = make_message(std::string("System booted."), ConsoleEndpoint{"stderr"});
    
    // Automatically resolves MessageProtocolService types from the variable context
    MessageProtocolService<StdIOTag, decltype(io_msg)> io_service;
    io_service.sendMessage(io_msg);

    // 2. Testing with alternative datatypes (int content / WebHook structure)
    auto hook_msg = make_message(503, WebHookEndpoint{"https://site.com", "auth_token_xyz"});
    MessageProtocolService<WebHookTag, decltype(hook_msg)> hook_service;
    hook_service.sendMessage(hook_msg);

    // 3. File Stream Implementation Demo
    std::ofstream log_file("application_audit.log", std::ios::app);
    if (log_file.is_open()) {
        
        // Factory deduces complex reference wrappers smoothly
        auto file_msg = make_message(std::string("Security intrusion alert!"), FileStreamEndpoint{log_file});

        MessageProtocolService<FileStreamTag, decltype(file_msg)> file_service;
        file_service.sendMessage(file_msg);
        
        log_file.close();
    } else {
        std::cerr << "Failed to open local logging file handle." << std::endl;
    }

    return 0;
}
