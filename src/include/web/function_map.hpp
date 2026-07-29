// NEED TO CREATE A FUNCTION MAP FOR THE SERVER TO CALL CLIENT FUNCTIONS, AND VICE VERSA, WITHOUT TIGHT COUPLING.
// THIS WILL ALLOW US TO EASILY ADD NEW FUNCTIONALITY WITHOUT MODIFYING THE CORE SERVER/CLIENT LOGIC.
#include <boost/uuid/uuid.hpp>
#include <boost/uuid/uuid_io.hpp>
#include <iostream>
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

// FIX: Account structural components match your constructor layout
std::unordered_map<uuid_T, Account> accountTable{
    {uuidTable[0], Account{loginCredentials{"ADMIN", "PASSWD"}, uuidTable[0]}}
}; 

std::flat_map<std::string, uuid_T> nameToUUID = {{"Admin", uuidTable[0]}};
std::unordered_map<uuid_T, Account> tombstonedAccountsTable = {{uuidTable[0], accountTable[uuidTable[0]]}};

void login(loginRequest& logReq) { // Use reference to preserve status/uuid updates
    switch (logReq.cmd) {
        case LogCmd::LOGIN: {
            std::println("Logging in as {}...", logReq.logCredentials.username);
            
            // 1. Get a view over just the map's values
            auto acctValues = accountTable | std::views::values;
            
            // 2. Search for matching credentials
            auto it = std::ranges::find_if(acctValues, [&logReq](const auto& acc) {
                return acc.loginCred.username == logReq.logCredentials.username;
            });
            
            if (it != acctValues.end() && logReq.logCredentials.password == it->loginCred.password) {
                logReq.uuid = it->uuid;
                
                // 3. Generate token
                std::string generatedToken = account::generateJWTSessionToken(logReq.logCredentials.username);
                
                // 4. Update the storage location inside the map securely via working iterator
                it->sessionToken = generatedToken;
                
                // 5. MEMORY SAFETY FIX: Store a clean pointer reference safely through the live iterator match
                session.account = &(*it); 
                session.sessiontoken = generatedToken;
                
                activeSession.push_back(generatedToken);
                logReq.status = LogResult::SUCCESS;
                
                std::println("Login Successful. Name:{0} UUID:{1}\nsession:{2}", 
                             logReq.logCredentials.username, boost::uuids::to_string(logReq.uuid), session.account->sessionToken);
            } 
            else {
                std::println("Username not found or invalid password.");
                logReq.status = LogResult::FAILURE;
            }
            break;
        }

        case LogCmd::CREATE: { 
            std::println("Creating New Account:\t{}...", logReq.logCredentials.username);
            
            auto acctCreds = accountTable 
                           | std::views::values 
                           | std::views::transform([](const auto& acc) -> const auto& { return acc.loginCred; });
            
            auto it = std::ranges::find_if(acctCreds, [&logReq](const auto& cred) {
                return cred.username == logReq.logCredentials.username;
            });

            if (it == acctCreds.end()) {
                Account newAccount = account::createAccount(logReq.logCredentials, uuidTable);
                logReq.uuid = newAccount.uuid; 
                
                accountTable.emplace(newAccount.uuid, newAccount);
                nameToUUID[newAccount.loginCred.username] = newAccount.uuid;
                logReq.status = LogResult::SUCCESS;
                
                std::println("Successfully Created Account. Name: {0} UUID {1}", logReq.logCredentials.username, boost::uuids::to_string(logReq.uuid));
            } 
            else {
                std::println("Account Creation Failed: Username already exists.");
                logReq.status = LogResult::FAILURE;
            }
            break;
        }

        case LogCmd::LOGOUT: {
            std::println("{0} is Logging out...", logReq.logCredentials.username);
            
            // Clean up session allocations securely upon explicit logout instructions
            if (session.account && session.account->uuid == logReq.uuid) {
                std::erase(activeSession, session.sessiontoken);
                session.account = nullptr;
                session.sessiontoken = "";
            }
            logReq.status = LogResult::SUCCESS;
            break;
        }

        case LogCmd::DEL: {
            std::println("Processing deletion request for: {}", logReq.logCredentials.username);
            
            auto it = accountTable.find(logReq.uuid);
            
            if (it == accountTable.end()) {
                std::println("Invalid Deletion Request: Account ID does not exist.");
                logReq.status = LogResult::FAILURE;
                break;
            }

            if (it->second.loginCred.username != logReq.logCredentials.username) {
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
            } else {
                std::println("Deletion cancelled by administrative prompt.\n");
                logReq.status = LogResult::FAILURE;
            }
            break;
        }
        default: 
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
