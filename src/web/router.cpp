#include <BPY/util.hpp>
#include <web/datahandler.hpp>
#include <web/loginhandler.hpp>
#include <web/router.hpp>
#include <web/log.hpp>
#include <sys/types.h>
#include <iostream>    
#include <print>      

using RequestVariant = std::variant<loginRequest, data::dataTransaction>;

void handleLogin(loginRequest &req) {    
    login(req);
//    logBuilder.push_back(std::format("T:{:%Y-%m-%d %H:%M:%S} -- {}",std::chrono::system_clock::now(), (req.status==LogResult::SUCCESS)? "NOTIFY:Login Request Handler Success": "ERROR:Login Request Handler Failed"));
}

void handleDataRequest(data::dataTransaction& request) {
    handleDataSync(request); 
  //  dataLogBuilder.push_back(std::format("T:{:%Y-%m-%d %H:%M:%S} -- {}",std::chrono::system_clock::now(), (request.status==data::Result::SUCCESS)? "NOTIFY:DataRequest Handler Success": "ERROR:DataRequest Handler Failed"));
}

void dispatch(RequestType cmd,std::variant<loginRequest, data::dataTransaction>& req) {
switch (cmd) {
    case RequestType::LOGIN: 
        if (auto login_req = std::get_if<loginRequest>(&req))handleLogin(*login_req);
      //  else {logBuilder.push_back(std::format("T:{:%Y-%m-%d %H:%M:%S} -- ERROR: Payload is not a loginRequest.",std::chrono::system_clock::now()));}
        break;
    
    case RequestType::DATA: 
        if (auto data_req = std::get_if<data::dataTransaction>(&req)) handleDataRequest(*data_req);
      //  else {dataLogBuilder.push_back(std::format("T:{:%Y-%m-%d %H:%M:%S} -- ERROR: Payload is not a DataRequest.",std::chrono::system_clock::now()));}
        break;
    
    };
}
