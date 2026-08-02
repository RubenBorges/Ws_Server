#include <web/loginhandler.hpp>
#include <log.hpp>
#include <print>
#include <cctype>

// Implement Case-Insensitive Matching
bool operator==(const loginCredentials &lhs, const loginCredentials &rhs) {
  std::string leftUser = lhs.username;
  std::string rightUser = rhs.username;
  std::transform(leftUser.begin(), leftUser.end(), leftUser.begin(), [](unsigned char c){ return std::tolower(c); });
  std::transform(rightUser.begin(), rightUser.end(), rightUser.begin(), [](unsigned char c){ return std::tolower(c); });
  return leftUser == rightUser && lhs.password == rhs.password;
}

// Global Single Instance Definitions 
loginRequest ThisLoginRequest;
session_t session;

namespace account {

boost::uuids::uuid generate_any_uuid() {
  static boost::uuids::random_generator gen;
  return gen();
}

boost::uuids::uuid generate_uuid(std::span<boost::uuids::uuid> idTable) {
  boost::uuids::uuid id;
  do {
    id = generate_any_uuid();
  } while (!is_new<boost::uuids::uuid>(idTable, id));
  return id;
}

Account createAccount(const loginCredentials &logInput, std::vector<boost::uuids::uuid> &uuidTable) {
  std::span<boost::uuids::uuid> uuidSpan = uuidTable;
  boost::uuids::uuid newUuid = generate_uuid(uuidSpan);
  Account acc{logInput, newUuid, "", true};
  uuidTable.emplace_back(acc.uuid);
  return acc;
}

std::string generateSecureSessionToken() {
  static boost::uuids::basic_random_generator<boost::random_device> secureGen;
  boost::uuids::uuid token = secureGen();
  return boost::uuids::to_string(token);
}

std::string generateJWTSessionToken(const std::string &username, const std::string &uuidStr) {
  auto token = jwt::create()
                   .set_issuer("auth_server")
                   .set_type("JWS")
                   .set_issued_at(std::chrono::system_clock::now())
                   .set_expires_at(std::chrono::system_clock::now() + std::chrono::hours(24))
                   .set_payload_claim("username", jwt::claim(username))
                   .set_payload_claim("uuid", jwt::claim(uuidStr))
                   .sign(jwt::algorithm::hs256{"YOUR_SECRET_CRYPTO_KEY"});
  return token;
}

bool validateLogin(const Account &acc, const loginCredentials &logInput) {
  return acc.loginCred == logInput;
}

} // namespace account

// Define and initialize table states safely in exactly one compiled object
std::vector<sessionToken_t> activeSession;
std::vector<uuid_T> uuidTable = {account::generate_any_uuid(), account::generate_any_uuid()};

std::unordered_map<uuid_T, Account> accountTable{
  {uuidTable[0], Account{loginCredentials{"Admin", "PASSWD"}, uuidTable[0]}}
};

std::flat_map<std::string, uuid_T> nameToUUID = {{"Admin", uuidTable[0]}};
std::unordered_map<uuid_T, Account> tombstonedAccountsTable = {{uuidTable[0], accountTable[uuidTable[0]]}};

std::string user = "Admin";
int userId = 101;
double latency = 14.25;

void login(loginRequest &logReq) {
  switch (logReq.cmd) {
    case LogCmd::LOGIN: {
      logBuilder.push_back(std::format("User: {} (ID: {})", user, userId));
      logBuilder.push_back(std::format("Status: SUCCESS | Latency: {:.2f}ms", latency));
      logBuilder.push_back(std::format("T:{:%Y-%m-%d %H:%M:%S} -- NOTIFY:Login Attempt. Name:{}", 
                                       std::chrono::system_clock::now(), logReq.logCredentials.username));
      
      std::println("Logging in as {}...", logReq.logCredentials.username);

      auto it = std::ranges::find_if(accountTable, [&logReq](const auto &pair) {
        return pair.second.loginCred == logReq.logCredentials;
      });

      if (it != accountTable.end()) {
        logReq.uuid = it->second.uuid;
        std::string generatedToken = account::generateJWTSessionToken(logReq.logCredentials.username);
        it->second.sessionToken = generatedToken;

        session.account = &(it->second);
        session.sessiontoken = generatedToken;
        activeSession.push_back(generatedToken);
        logReq.status = LogResult::SUCCESS;

        logBuilder.push_back(std::format("T:{:%Y-%m-%d %H:%M:%S} -- NOTIFY:Login Successful. Name:{} UUID:{} Session Token Generated: {}",
                                         std::chrono::system_clock::now(), logReq.logCredentials.username,
                                         boost::uuids::to_string(logReq.uuid), session.account->sessionToken));
        
        std::println("Login Successful. Name:{0} UUID:{1}\nSession Token Generated: {2}",
                     logReq.logCredentials.username, boost::uuids::to_string(logReq.uuid), session.account->sessionToken);
      } else {
        std::println("Username not found or invalid password.");
        logBuilder.push_back(std::format("T:{:%Y-%m-%d %H:%M:%S} -- ERROR: Username [{}] not found or invalid password.",
                                         std::chrono::system_clock::now(), logReq.logCredentials.username));
        logReq.status = LogResult::FAILURE;
      }
      break;
    }
    case LogCmd::CREATE: {
      logBuilder.push_back(std::format("T:{:%Y-%m-%d %H:%M:%S} -- NOTIFY:Account Creation Attempt. Name:{}",
                                       std::chrono::system_clock::now(), logReq.logCredentials.username));
      std::println("Creating New Account:\t{}...", logReq.logCredentials.username);

      auto it = std::ranges::find_if(accountTable, [&logReq](const auto &pair) {
        return pair.second.loginCred == logReq.logCredentials;
      });

      if (it == accountTable.end()) {
        Account newAccount = account::createAccount(logReq.logCredentials, uuidTable);
        logReq.uuid = newAccount.uuid;
        accountTable.emplace(newAccount.uuid, newAccount);
        nameToUUID[newAccount.loginCred.username] = newAccount.uuid;
        logReq.status = LogResult::SUCCESS;

        logBuilder.push_back(std::format("T:{:%Y-%m-%d %H:%M:%S} -- NOTICE: Successfully Created Account. Name: {} UUID {}",
                                         std::chrono::system_clock::now(), logReq.logCredentials.username, boost::uuids::to_string(logReq.uuid)));
        std::println("Successfully Created Account. Name: {0} UUID {1}",
                     logReq.logCredentials.username, boost::uuids::to_string(logReq.uuid));
      } else {
        std::println("Account Creation Failed: Username already exists.");
        logBuilder.push_back(std::format("T:{:%Y-%m-%d %H:%M:%S} -- ERROR: Account Creation Failed: Username [{}] already exists.",
                                         std::chrono::system_clock::now(), logReq.logCredentials.username));
        logReq.status = LogResult::FAILURE;
      }
      break;
    }
    
    case LogCmd::LOGOUT: {
      std::println("{0} is Logging out...", logReq.logCredentials.username);
      logBuilder.push_back(std::format(
          "T:{:%Y-%m-%d %H:%M:%S} -- NOTIFY:Logout Attempt. Name:{}",
          std::chrono::system_clock::now(), logReq.logCredentials.username));

      if (session.account && session.account->uuid == logReq.uuid) {
        std::erase(activeSession, session.sessiontoken);
        session.account = nullptr;
        session.sessiontoken = "";
      }

      logBuilder.push_back(std::format(
          "T:{:%Y-%m-%d %H:%M:%S} -- NOTIFY:Logout Success. Name:{}",
          std::chrono::system_clock::now(), logReq.logCredentials.username));
      logReq.status = LogResult::SUCCESS;
      break;
    }

    case LogCmd::DEL: {
      std::println("Processing deletion request for: {}", logReq.logCredentials.username);
      logBuilder.push_back(std::format(
          "T:{:%Y-%m-%d %H:%M:%S} -- NOTIFY:Processing deletion request for: {}",
          std::chrono::system_clock::now(), logReq.logCredentials.username));

      auto it = accountTable.find(logReq.uuid);
      if (it == accountTable.end()) {
        std::println("Invalid Deletion Request: Account ID does not exist.");

        logBuilder.push_back(std::format(
            "T:{:%Y-%m-%d %H:%M:%S} -- ERROR:Invalid Deletion Request: Account [{}] does not exist.",
            std::chrono::system_clock::now(), logReq.logCredentials.username));
        logReq.status = LogResult::FAILURE;
        break;
      }
      
      if (it->second.loginCred.username != logReq.logCredentials.username) {
        logBuilder.push_back(std::format(
            "T:{:%Y-%m-%d %H:%M:%S} -- ERROR:Security Violation: Username [{}] mismatch for target UUID.",
            std::chrono::system_clock::now(), logReq.logCredentials.username));
        std::println("Security Violation: Username mismatch for target UUID.");
        logReq.status = LogResult::FAILURE;
        break;
      }

      char promptReply = 'N';
      std::println("Deleting Account:\t{}", it->second.loginCred.username);
      std::println("Enter Y to continue deletion or any other key to cancel request...");
      
      // FIXED: Added safe stream extraction processing to prevent crashes if std::cin encounters EOF or error flags
      if (!(std::cin >> promptReply)) {
        std::cin.clear(); // Reset failure flags
        promptReply = 'N';
      }

      if (promptReply == 'y' || promptReply == 'Y') {
        tombstonedAccountsTable.emplace(it->first, it->second);
        it->second.loginCred.password = "TOMBSTONE";
        it->second.active = false;
        nameToUUID.erase(it->second.loginCred.username);
        logReq.status = LogResult::SUCCESS;
        std::println("Deletion Completed Successfully.\n");
        logBuilder.push_back(
            std::format("T:{:%Y-%m-%d %H:%M:%S} -- NOTIFY: Deletion Successful",
                        std::chrono::system_clock::now()));
      } else {
        std::println("Deletion cancelled by administrative prompt.\n");
        logBuilder.push_back(
            std::format("T:{:%Y-%m-%d %H:%M:%S} -- NOTIFY:Deletion cancelled by administrative prompt.",
                        std::chrono::system_clock::now()));
        logReq.status = LogResult::FAILURE;
      }
      break;
    }
    
    default: {
      logBuilder.push_back(
          std::format("T:{:%Y-%m-%d %H:%M:%S} -- ERROR:Log Command Switch Error",
                      std::chrono::system_clock::now()));
      std::cerr << "Log Command Switch Error\n";
      break;
    }
  }
}