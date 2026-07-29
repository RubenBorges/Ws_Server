// NEED TO CREATE A FUNCTION MAP FOR THE SERVER TO CALL CLIENT FUNCTIONS, AND VICE VERSA, WITHOUT TIGHT COUPLING.
// THIS WILL ALLOW US TO EASILY ADD NEW FUNCTIONALITY WITHOUT MODIFYING THE CORE SERVER/CLIENT LOGIC.
#include <boost/uuid/uuid.hpp>
#include <boost/uuid/uuid_io.hpp>
#include <iostream>
#include <format>
#include <print>
#include <string>
#include <unordered_map>
#include <flat_map>
#include <utility>
#include <tuple>
#include "login.hpp"
#include <cstdint>
#include <algorithm>
#include <chrono>
#include <ranges>
#include <functional> // Required for type-safe std::function closures

using uuid_T = boost::uuids::uuid;

// Global state tables
std::vector<sessionToken_t> activeSession;
std::vector<uuid_T> uuidTable = {account::generate_any_uuid(), account::generate_any_uuid()};
std::vector<std::string> logBuilder(5);
// FIX: Account structural components match your constructor layout
std::unordered_map<uuid_T, Account> accountTable{
    {uuidTable[0], Account{loginCredentials{"ADMIN", "PASSWD"}, uuidTable[0]}}
}; 

std::flat_map<std::string, uuid_T> nameToUUID = {{"Admin", uuidTable[0]}};
std::unordered_map<uuid_T, Account> tombstonedAccountsTable = {{uuidTable[0], accountTable[uuidTable[0]]}};
std::string user = "Admin";
    int userId = 101;
    double latency = 14.25;

    // Push back formatted strings cleanly
    void login(loginRequest& logReq) { // Use reference to preserve status/uuid updates
        switch (logReq.cmd) {
        logBuilder.push_back(std::format("User: {} (ID: {})", user, userId));
        logBuilder.push_back(std::format("Status: SUCCESS | Latency: {:.2f}ms", latency));
        
        case LogCmd::LOGIN: {
            logBuilder.push_back(std::format("T:{0} -- NOTIFY:Login Attempt. Name:{1}", std::chrono::system_clock::now(), logReq.logCredentials.username));
            std::println("Logging in as {}...", logReq.logCredentials.username);

            // This bypasses the elements_view entirely so 'it' is a real map iterator.
            auto it = std::ranges::find_if(accountTable, [&logReq](const auto& pair) {
                return pair.second.loginCred.username == logReq.logCredentials.username;
            });
            
            // Use standard map syntax (it->second) to check the password safely
            if (it != accountTable.end() && logReq.logCredentials.password == it->second.loginCred.password) {
                logReq.uuid = it->second.uuid;
                
                std::string generatedToken = account::generateJWTSessionToken(logReq.logCredentials.username);
                
                // Update the stable account inside the map
                it->second.sessionToken = generatedToken;
                
                // Securely save the pointer via the map's stable node address
                session.account = &(it->second); 
                session.sessiontoken = generatedToken;
                
                activeSession.push_back(generatedToken);
                logReq.status = LogResult::SUCCESS;
                logBuilder.push_back(std::format("T:{0} -- NOTIFY:Login Successful. Name:{0} UUID:{1} Session Token Generated: {2}", std::chrono::system_clock::now(), logReq.logCredentials.username, boost::uuids::to_string(logReq.uuid), session.account->sessionToken));

                std::println("Login Successful. Name:{0} UUID:{1}\nSession Token Generated: {2}", 
                             logReq.logCredentials.username, boost::uuids::to_string(logReq.uuid), session.account->sessionToken);
            } 
            else {
                std::println("Username not found or invalid password.");
                logBuilder.push_back(std::format("T:{0} -- ERROR: Username [{1}] not found or invalid password.", std::chrono::system_clock::now(), logReq.logCredentials.username));
                logReq.status = LogResult::FAILURE;
            }
            break;
        }

        case LogCmd::CREATE: { 
            logBuilder.push_back(std::format("T:{0} -- NOTIFY:Account Creation Attempt. Name:{1}", std::chrono::system_clock::now(), logReq.logCredentials.username));
            std::println("Creating New Account:\t{}...", logReq.logCredentials.username);
            auto it = std::ranges::find_if(accountTable, [&logReq](const auto& pair) {
                return pair.second.loginCred.username == logReq.logCredentials.username;
            });

            // If it equals end, the username is unique and does not exist yet
            if (it == accountTable.end()) {
                Account newAccount = account::createAccount(logReq.logCredentials, uuidTable);
                logReq.uuid = newAccount.uuid; 
                accountTable.emplace(newAccount.uuid, newAccount);
                nameToUUID[newAccount.loginCred.username] = newAccount.uuid;
                logReq.status = LogResult::SUCCESS;
                logBuilder.push_back(std::format("T:{0} -- NOTICE: Successfully Created Account. Name: {1} UUID {2}", std::chrono::system_clock::now(), logReq.logCredentials.username, boost::uuids::to_string(logReq.uuid)));
            } 
            else {
                std::println("Account Creation Failed: Username already exists.");
                logBuilder.push_back(std::format("T:{0} -- ERROR: Account Creation Failed: Username [{1}] already exists. ", std::chrono::system_clock::now(), logReq.logCredentials.username));
                logReq.status = LogResult::FAILURE;
            }
            break;
        }
  case LogCmd::LOGOUT: {
            std::println("{0} is Logging out...", logReq.logCredentials.username);
            logBuilder.push_back(std::format("T:{0} -- NOTIFY:Logout Attempt. Name:{1}", std::chrono::system_clock::now(), logReq.logCredentials.username));
            // Clean up session allocations securely upon explicit logout instructions
            if (session.account && session.account->uuid == logReq.uuid) {
                std::erase(activeSession, session.sessiontoken);
                session.account = nullptr;
                session.sessiontoken = "";
            }
            logBuilder.push_back(std::format("T:{0} -- NOTIFY:Logout Success. Name:{1}", std::chrono::system_clock::now(), logReq.logCredentials.username));
            logReq.status = LogResult::SUCCESS;
            break;
        }

        case LogCmd::DEL: {
            std::println("Processing deletion request for: {}", logReq.logCredentials.username);
            logBuilder.push_back(std::format("T:{0} -- NOTIFY:Processing deletion request for: {1}", std::chrono::system_clock::now(), logReq.logCredentials.username));
            auto it = accountTable.find(logReq.uuid);
            if (it == accountTable.end()) {
                std::println("Invalid Deletion Request: Account ID does not exist.");
                logBuilder.push_back(std::format("T:{0} -- ERROR:Invalid Deletion Request: Account [{1}] does not exist.", std::chrono::system_clock::now(), logReq.logCredentials.username));
                logReq.status = LogResult::FAILURE;
                break;
            }

            if (it->second.loginCred.username != logReq.logCredentials.username) {
                logBuilder.push_back(std::format("T:{0} -- ERROR:Security Violation: Username [{1}] mismatch for target UUID.", std::chrono::system_clock::now(), logReq.logCredentials.username));
                std::println("Security Violation: Username mismatch for target UUID.");
                logReq.status = LogResult::FAILURE;
                break;
            }

            char promptReply;
            std::println("Deleting Account:\t{}", it->second.loginCred.username);
            std::println("Enter Y to continue deletion or any other key to cancel request...");
            std::cin >> promptReply;
            
            if (promptReply == 'y' || promptReply == 'Y') {
                tombstonedAccountsTable.emplace(it->first, it->second);
                
                it->second.loginCred.password = "TOMBSTONE";
                it->second.active = false;
                
                nameToUUID.erase(it->second.loginCred.username);
                logReq.status = LogResult::SUCCESS;
                std::println("Deletion Completed Successfully.\n");
                logBuilder.push_back(std::format("T:{0} -- NOTIFY: Deletion Successful", std::chrono::system_clock::now()));
            } else {
                std::println("Deletion cancelled by administrative prompt.\n");
                logBuilder.push_back(std::format("T:{0} -- NOTIFY:Deletion cancelled by administrative prompt.", std::chrono::system_clock::now()));

                logReq.status = LogResult::FAILURE;
            }
            break;
        }
        default: 
            logBuilder.push_back(std::format("T:{0} -- ERROR:Log Command Switch Error", std::chrono::system_clock::now()));
            std::cerr << "Log Command Switch Error";
    }
}

// Handles a login command placeholder for the server-side command dispatcher.
void handleLogin(loginRequest& req) {
    login(req);
}

// Handles a data-processing command placeholder for the server-side dispatcher.
void handleData(loginRequest& req) { 
    std::println("Processing data payload requests for UUID: {}", boost::uuids::to_string(req.uuid)); 
}

// MODERNIZED FIX: Replaced raw function pointers with decoupled std::function signatures passing requests by reference
static const std::unordered_map<std::string, std::function<void(loginRequest&)>> functionTable = {
    {"login", handleLogin},
    {"data", handleData}
};

// Looks up a command name in the function table and invokes the matching handler with input payload state references
void dispatch(const std::string& cmd, loginRequest& req) {
    // Safe lookup method instead of using operator[] which introduces garbage collection insertions
    auto it = functionTable.find(cmd);
    
    if (it != functionTable.end()) {
        it->second(req); // Invoke type-safe dynamic payload functor execution loop paths
    } else {
        std::println(std::cerr, "Command: '{}' not supported by endpoint routing matrices.", cmd);
    }
}
