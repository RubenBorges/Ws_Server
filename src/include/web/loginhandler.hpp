#pragma once // Prevents recursive header inclusion bugs

#include <algorithm>
#include <boost/random/random_device.hpp>
#include <boost/uuid/uuid.hpp>
#include <boost/uuid/uuid_generators.hpp>
#include <boost/uuid/uuid_io.hpp>
#include <flat_map>
#include <jwt-cpp/jwt.h>
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
