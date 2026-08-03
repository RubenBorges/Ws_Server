#include <BPY/util.hpp>
#include <cstdlib>
#include <RequestRouter/router.hpp>
#include <sys/types.h>
#include <log.hpp>

using RequestVariant = std::variant<loginRequest, data::dataTransaction>;

namespace router{
  int dispatch(RequestType cmd,std::variant<loginRequest, data::dataTransaction>& req) {
    switch (cmd) {
      case RequestType::LOGIN: 
        if (auto login_req = std::get_if<loginRequest>(&req))login(*login_req);
        else {logBuilder.push_back(std::format("T:{:%Y-%m-%d %H:%M:%S} -- ERROR: Payload is not a loginRequest.",std::chrono::system_clock::now()));}
        break;
      case RequestType::DATA: 
        if (auto data_req = std::get_if<data::dataTransaction>(&req)) handleDataSync(*data_req);
        else {logBuilder.push_back(std::format("T:{:%Y-%m-%d %H:%M:%S} -- ERROR: Payload is not a DataRequest.",std::chrono::system_clock::now()));}
        break;
      default:
        return EXIT_FAILURE;
      }
    return EXIT_SUCCESS;
  }

  int DispatchRequest(RequestType reqType, RequestVariant &payload) {
    return dispatch(reqType, payload );
  }
} //namespace router