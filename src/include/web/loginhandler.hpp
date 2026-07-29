#pragma once // Prevents recursive header inclusion bugs

#pragma once

#include <algorithm>
#include <boost/random/random_device.hpp>
#include <boost/uuid/uuid.hpp>
#include <boost/uuid/uuid_generators.hpp>
#include <boost/uuid/uuid_io.hpp>
#include <chrono>
#include <flat_map>
#include <format>
#include <iostream>
#include <jwt-cpp/jwt.h>
#include <print>
#include <span>
#include <string>
#include <unordered_map>
#include <vector>

using uuid_T = boost::uuids::uuid;
using sessionToken_t = std::string;

enum class LogCmd : int { LOGIN = 0, CREATE = 1, LOGOUT = 2, DEL = 4 };
enum class LogResult : int { SUCCESS = 0, FAILURE = 1, PENDING = 2 };

struct loginCredentials {
  std::string username;
  std::string password;
  friend bool operator==(const loginCredentials &lhs, const loginCredentials &rhs);
};

struct loginRequest {
  LogCmd cmd;
  loginCredentials logCredentials;
  uuid_T uuid;
  LogResult status{LogResult::PENDING};
};

struct Account {
  using uuid_t = boost::uuids::uuid;
  using sessionToken_t = std::string;
  loginCredentials loginCred;
  uuid_t uuid;
  sessionToken_t sessionToken{""};
  bool active{true};
};

struct session_t {
  sessionToken_t sessiontoken;
  Account *account{nullptr};
};

// Share instances across different code files safely via extern
extern loginRequest ThisLoginRequest;
extern session_t session;

namespace account {

template <typename T> 
bool is_new(std::span<T> range, const T &testTarget) {
  auto it = std::ranges::find_if(
      range, [&testTarget](const T &id) { return id == testTarget; });
  return it == range.end();
}

boost::uuids::uuid generate_any_uuid();
boost::uuids::uuid generate_uuid(std::span<boost::uuids::uuid> idTable);
Account createAccount(const loginCredentials &logInput, std::vector<boost::uuids::uuid> &uuidTable);
std::string generateSecureSessionToken();
std::string generateJWTSessionToken(const std::string &username, const std::string &uuidStr = generateSecureSessionToken());
bool validateLogin(const Account &acc, const loginCredentials &logInput);

} // namespace account

// Declare shared global state tables
extern std::vector<sessionToken_t> activeSession;
extern std::vector<uuid_T> uuidTable;
extern std::vector<std::string> logBuilder;
extern std::unordered_map<uuid_T, Account> accountTable;
extern std::flat_map<std::string, uuid_T> nameToUUID;
extern std::unordered_map<uuid_T, Account> tombstonedAccountsTable;

// Diagnostics
extern std::string user;
extern int userId;
extern double latency;

// Primary execution path
void login(loginRequest &logReq);


// #include <algorithm>
// #include <boost/random/random_device.hpp>
// #include <boost/uuid/uuid.hpp>
// #include <boost/uuid/uuid_generators.hpp>
// #include <boost/uuid/uuid_io.hpp>
// #include <chrono>
// #include <flat_map>
// #include <format>
// #include <iostream>
// #include <jwt-cpp/jwt.h>
// #include <print>
// #include <span>
// #include <string>
// #include <unordered_map>
// #include <vector>

// using uuid_T = boost::uuids::uuid;
// using sessionToken_t = std::string;

// enum class LogCmd : int { LOGIN = 0 , CREATE = 1, LOGOUT = 2, DEL = 4 };
// enum class LogResult : int { SUCCESS = 0, FAILURE = 1, PENDING = 2 };

// struct loginCredentials {
//   std::string username;
//   std::string password;
//   friend bool operator==(const loginCredentials &lhs, const loginCredentials &rhs) {
//     return lhs.username == rhs.username && lhs.password == rhs.password;
//   }
// };

// // User Account struct containing Login Info and UUID
// struct loginRequest {
//   LogCmd cmd;
//   loginCredentials logCredentials;
//   uuid_T uuid;
//   LogResult status{LogResult::PENDING};
// };

// struct Account {
//   using uuid_t = boost::uuids::uuid;
//   using sessionToken_t = std::string;
//   loginCredentials loginCred;
//   uuid_t uuid;
//   sessionToken_t sessionToken{""};
//   bool active{true};
// };

// struct session_t {
//   sessionToken_t sessiontoken;
//   Account *account{nullptr};
// };

// // Inline ensures global consistency without duplicate symbol linker bugs
// inline loginRequest ThisLoginRequest;
// inline session_t session;

// namespace account {

// template <typename T> bool is_new(std::span<T> range, const T &testTarget) {
//   auto it = std::ranges::find_if(
//       range, [&testTarget](const T &id) { return id == testTarget; });
//   return it == range.end();
// }

// inline boost::uuids::uuid generate_any_uuid() {
//   static boost::uuids::random_generator gen;
//   return gen();
// }

// inline boost::uuids::uuid generate_uuid(std::span<boost::uuids::uuid> idTable) {
//   boost::uuids::uuid id;
//   do {
//     id = generate_any_uuid();
//   } while (is_new<boost::uuids::uuid>(idTable, id) == false);
//   return id;
// }

// inline Account createAccount(const loginCredentials &logInput,
//                              std::vector<boost::uuids::uuid> &uuidTable) {
//   std::span<boost::uuids::uuid> uuidSpan = uuidTable;
//   boost::uuids::uuid newUuid = generate_uuid(uuidSpan);
//   Account acc{logInput, newUuid, "", true};
//   uuidTable.emplace_back(acc.uuid);
//   return acc;
// }

// inline std::string generateSecureSessionToken() {
//   static boost::uuids::basic_random_generator<boost::random_device> secureGen;
//   boost::uuids::uuid token = secureGen();
//   return boost::uuids::to_string(token);
// }

// inline std::string generateJWTSessionToken(
//     const std::string &username,
//     const std::string &uuidStr = generateSecureSessionToken()) {
//   auto token = jwt::create()
//                    .set_issuer("auth_server")
//                    .set_type("JWS")
//                    .set_issued_at(std::chrono::system_clock::now())
//                    .set_expires_at(std::chrono::system_clock::now() +
//                                    std::chrono::hours(24))
//                    .set_payload_claim("username", jwt::claim(username))
//                    .set_payload_claim("uuid", jwt::claim(uuidStr))
//                    .sign(jwt::algorithm::hs256{"YOUR_SECRET_CRYPTO_KEY"});
//   return token;
// }

// inline bool validateLogin(const Account &acc,const loginCredentials &logInput) {return acc.loginCred == logInput;}
// } // namespace account

// // Global state tables
// inline std::vector<sessionToken_t> activeSession;
// inline std::vector<uuid_T> uuidTable = {account::generate_any_uuid(),
//                                         account::generate_any_uuid()};
// inline std::vector<std::string>
//     logBuilder; // Fixed initialization (removed allocation size to avoid empty
//                 // strings)

//  inline std::unordered_map<uuid_T, Account> accountTable{{uuidTable[0], Account{loginCredentials{"Admin", "PASSWD"}, uuidTable[0]}}};

// inline std::flat_map<std::string, uuid_T> nameToUUID = {
//     {"Admin", uuidTable[0]}};
// inline std::unordered_map<uuid_T, Account> tombstonedAccountsTable = {
//     {uuidTable[0], accountTable[uuidTable[0]]}};

// // Mock variables for diagnostic tracing
// inline std::string user = "Admin";
// inline int userId = 101;
// inline double latency = 14.25;

// inline void login(loginRequest &logReq) {
//   switch (logReq.cmd) {
//   case LogCmd::LOGIN: {
//     // FIX: Safe placement of log statements inside the explicit case block
//     // context
//     logBuilder.push_back(std::format("User: {} (ID: {})", user, userId));
//     logBuilder.push_back(
//         std::format("Status: SUCCESS | Latency: {:.2f}ms", latency));

//     // Fixed chrono sub-formatting sequence mapping
//     logBuilder.push_back(std::format(
//         "T:{:%Y-%m-%d %H:%M:%S} -- NOTIFY:Login Attempt. Name:{}",
//         std::chrono::system_clock::now(), logReq.logCredentials.username));
//     std::println("Logging in as {}...", logReq.logCredentials.username);

//     auto it = std::ranges::find_if(accountTable, [&logReq](const auto &pair) {
//       return pair.second.loginCred.username == logReq.logCredentials.username;
//     });

//     if (it != accountTable.end() && account::validateLogin(accountTable[it->second.uuid],logReq.logCredentials)){
//       logReq.uuid = it->second.uuid;

//       std::string generatedToken =
//           account::generateJWTSessionToken(logReq.logCredentials.username);
//       it->second.sessionToken = generatedToken;

//       session.account = &(it->second);
//       session.sessiontoken = generatedToken;

//       activeSession.push_back(generatedToken);
//       logReq.status = LogResult::SUCCESS;

//       logBuilder.push_back(std::format(
//           "T:{:%Y-%m-%d %H:%M:%S} -- NOTIFY:Login Successful. Name:{} UUID:{} "
//           "Session Token Generated: {}",
//           std::chrono::system_clock::now(), logReq.logCredentials.username,
//           boost::uuids::to_string(logReq.uuid), session.account->sessionToken));

//       std::println(
//           "Login Successful. Name:{0} UUID:{1}\nSession Token Generated: {2}",
//           logReq.logCredentials.username, boost::uuids::to_string(logReq.uuid),
//           session.account->sessionToken);
//     } else {
//       std::println("Username not found or invalid password.");
//       logBuilder.push_back(std::format(
//           "T:{:%Y-%m-%d %H:%M:%S} -- ERROR: Username [{}] not found or invalid "
//           "password.",
//           std::chrono::system_clock::now(), logReq.logCredentials.username));
//       logReq.status = LogResult::FAILURE;
//     }
//     break;
//   }

//   case LogCmd::CREATE: {
//     logBuilder.push_back(std::format(
//         "T:{:%Y-%m-%d %H:%M:%S} -- NOTIFY:Account Creation Attempt. Name:{}",
//         std::chrono::system_clock::now(), logReq.logCredentials.username));
//     std::println("Creating New Account:\t{}...",
//                  logReq.logCredentials.username);

//     auto it = std::ranges::find_if(accountTable, [&logReq](const auto &pair) {
//       return pair.second.loginCred.username == logReq.logCredentials.username;
//     });

//     if (it == accountTable.end()) {
//       Account newAccount =
//           account::createAccount(logReq.logCredentials, uuidTable);
//       logReq.uuid = newAccount.uuid;
//       accountTable.emplace(newAccount.uuid, newAccount);
//       nameToUUID[newAccount.loginCred.username] = newAccount.uuid;
//       logReq.status = LogResult::SUCCESS;

//       logBuilder.push_back(std::format(
//           "T:{:%Y-%m-%d %H:%M:%S} -- NOTICE: Successfully Created Account. "
//           "Name: {} UUID {}",
//           std::chrono::system_clock::now(), logReq.logCredentials.username,
//           boost::uuids::to_string(logReq.uuid)));
//       std::println("Successfully Created Account. Name: {0} UUID {1}",
//                    logReq.logCredentials.username,
//                    boost::uuids::to_string(logReq.uuid));
//     } else {
//       std::println("Account Creation Failed: Username already exists.");
//       logBuilder.push_back(std::format(
//           "T:{:%Y-%m-%d %H:%M:%S} -- ERROR: Account Creation Failed: Username "
//           "[{}] already exists. ",
//           std::chrono::system_clock::now(), logReq.logCredentials.username));
//       logReq.status = LogResult::FAILURE;
//     }
//     break;
//   }

//   case LogCmd::LOGOUT: {
//     std::println("{0} is Logging out...", logReq.logCredentials.username);
//     logBuilder.push_back(std::format(
//         "T:{:%Y-%m-%d %H:%M:%S} -- NOTIFY:Logout Attempt. Name:{}",
//         std::chrono::system_clock::now(), logReq.logCredentials.username));

//     if (session.account && session.account->uuid == logReq.uuid) {
//       std::erase(activeSession, session.sessiontoken);
//       session.account = nullptr;
//       session.sessiontoken = "";
//     }

//     logBuilder.push_back(std::format(
//         "T:{:%Y-%m-%d %H:%M:%S} -- NOTIFY:Logout Success. Name:{}",
//         std::chrono::system_clock::now(), logReq.logCredentials.username));
//     logReq.status = LogResult::SUCCESS;
//     break;
//   }

//   case LogCmd::DEL: {
//     std::println("Processing deletion request for: {}",
//                  logReq.logCredentials.username);
//     logBuilder.push_back(std::format(
//         "T:{:%Y-%m-%d %H:%M:%S} -- NOTIFY:Processing deletion request for: {}",
//         std::chrono::system_clock::now(), logReq.logCredentials.username));

//     auto it = accountTable.find(logReq.uuid);
//     if (it == accountTable.end()) {
//       std::println("Invalid Deletion Request: Account ID does not exist.");

//       logBuilder.push_back(std::format(
//           "T:{:%Y-%m-%d %H:%M:%S} -- ERROR:Invalid Deletion Request: Account "
//           "[{}] does not exist.",
//           std::chrono::system_clock::now(), logReq.logCredentials.username));
//       logReq.status = LogResult::FAILURE;
//       break;
//     }
//     if (it->second.loginCred.username != logReq.logCredentials.username) {
//       logBuilder.push_back(std::format(
//           "T:{:%Y-%m-%d %H:%M:%S} -- ERROR:Security Violation: Username [{}] "
//           "mismatch for target UUID.",
//           std::chrono::system_clock::now(), logReq.logCredentials.username));
//       std::println("Security Violation: Username mismatch for target UUID.");
//       logReq.status = LogResult::FAILURE;
//       break;
//     }
//     char promptReply;
//     std::println("Deleting Account:\t{}", it->second.loginCred.username);
//     std::println(
//         "Enter Y to continue deletion or any other key to cancel request...");
//     std::cin >> promptReply;
//     if (promptReply == 'y' || promptReply == 'Y') {
//       tombstonedAccountsTable.emplace(it->first, it->second);
//       it->second.loginCred.password = "TOMBSTONE";
//       it->second.active = false;
//       nameToUUID.erase(it->second.loginCred.username);
//       logReq.status = LogResult::SUCCESS;
//       std::println("Deletion Completed Successfully.\n");
//       logBuilder.push_back(
//           std::format("T:{:%Y-%m-%d %H:%M:%S} -- NOTIFY: Deletion Successful",
//                       std::chrono::system_clock::now()));
//     } else {
//       std::println("Deletion cancelled by administrative prompt.\n");
//       logBuilder.push_back(
//           std::format("T:{:%Y-%m-%d %H:%M:%S} -- NOTIFY:Deletion cancelled by "
//                       "administrative prompt.",
//                       std::chrono::system_clock::now()));
//       logReq.status = LogResult::FAILURE;
//     }
//     break;
//   }
//   default:
//     logBuilder.push_back(
//         std::format("T:{:%Y-%m-%d %H:%M:%S} -- ERROR:Log Command Switch Error",
//                     std::chrono::system_clock::now()));
//     std::cerr << "Log Command Switch Error";
//   }
// }
