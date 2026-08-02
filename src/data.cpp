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

using RequestVariant = std::variant<loginRequest, data::dataTransaction>;

int BuildRequest(RequestType reqType, RequestVariant &payload) {
    router::dispatch(reqType, payload );
    return 0;
}