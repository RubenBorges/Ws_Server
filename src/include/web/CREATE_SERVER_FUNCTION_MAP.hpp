// //NEED TO CREATE A FUNCTION MAP FOR THE SERVER TO CALL CLIENT FUNCTIONS, AND VICE VERSA, WITHOUT TIGHT COUPLING. THIS WILL ALLOW US TO EASILY ADD NEW FUNCTIONALITY WITHOUT MODIFYING THE CORE SERVER/CLIENT LOGIC.
// #include <boost/uuid/uuid.hpp>
// #include <boost/uuid/uuid_io.hpp>
// #include <iostream>
// #include <print>
// #include <string>
// #include <unordered_map>
// #include <flat_map>
// #include <utility>
// #include <tuple>
// #include "login.hpp"
// #include <cstdint>
// #include <algorithm>
// #include <chrono>
// #include <ranges>

// using uuid_T = boost::uuids::uuid;
// using sessionToken_t = std::string;
// enum class LogCmd : int{LOGIN, CREATE, LOGOUT, DEL};
// enum class LogResult : int{SUCCESS = 0, FAILURE = 1, PENDING = 2};

// struct loginRequest{
//     LogCmd cmd;
//     LogResult status{LogResult::PENDING};
//     loginCredentials logCredentials;
//     uuid_T uuid;
// };

// struct session_t{
//     sessionToken_t sessiontoken;
//     Account* account{nullptr};
// };
// std::vector<sessionToken_t> activeSession;
// std::vector<uuid_T> uuidTable = {account::generate_any_uuid(),account::generate_any_uuid()};
// std::unordered_map<uuid_T, Account> accountTable{{uuidTable[0],Account{"ADMIN","PASSWD",uuidTable[0]}}}; 
// std::flat_map< std::string,uuid_T> nameToUUID = {{"Admin",uuidTable[0]}};
// std::unordered_map<uuid_T, Account> tombstonedAccountsTable = { {uuidTable[0], accountTable[uuidTable[0]]}};

// static loginRequest ThisLoginRequest;
// static session_t session;

// void login(loginRequest& logReq){ // Use reference to preserve status/uuid updates
//     switch (logReq.cmd){
//         case LogCmd::LOGIN: {
//             std::println("Logging in as {}...", logReq.logCredentials.username);
//             auto acctCreds = accountTable | std::views::values | std::views::transform([](const auto& acc) -> const auto& {return acc;});
//             auto it = std::ranges::find_if(acctCreds,[&logReq](const auto& cred) {
//                     return cred.loginCred.username == logReq.logCredentials.username;
//                 });
//             if (it != acctCreds.end() && logReq.logCredentials.password == (*it).loginCred.password ){
//                 logReq.uuid = (*it).uuid;
//                 session.account = &accountTable[(*it).uuid];
//                 session.account->sessionToken = session.sessiontoken = account::generateJWTSessionToken(logReq.logCredentials.username);
//                 logReq.status = LogResult::SUCCESS;
//                 std::println("Login Successful. Name:{0} UUID:{1}\nsession:{2} ", logReq.logCredentials.username, boost::uuids::to_string(logReq.uuid),session.account->sessionToken);
//             } 
//             else {
//                 std::println("Username not found or invalid password.");
//                 logReq.status = LogResult::FAILURE;
//             }
//             break;
//         }

//         case LogCmd::CREATE: { 
//             std::println("Creating New Account:\t{}...", logReq.logCredentials.username);
            
//             auto acctCreds = accountTable 
//                            | std::views::values 
//                            | std::views::transform([](const auto& acc) -> const auto& { return acc.loginCred; });
            
//             auto it = std::ranges::find_if(acctCreds, [&logReq](const auto& cred) {
//                 return cred.username == logReq.logCredentials.username;
//             });

//             if (it == acctCreds.end()) {
//                 // 1. Generate the account using your utility helper
//                 Account newAccount = account::createAccount(logReq.logCredentials, uuidTable);
//                 // 2. Capture the newly generated UUID to populate your request payload
//                 logReq.uuid = newAccount.uuid; 
//                 // 3. FIXED MAP INSERTION: Pass both the key (UUID) and the value (Account object)
//                 accountTable.emplace(newAccount.uuid, newAccount);
//                 // 4. SYNCHRONIZE AUXILIARY MAPS: Update your name-to-UUID cache
//                 nameToUUID[newAccount.loginCred.username] = newAccount.uuid;
//                 logReq.status = LogResult::SUCCESS;
//                 std::println("Successfully Created Account. Name: {0} UUID {1}", logReq.logCredentials.username, boost::uuids::to_string(logReq.uuid));
//             } 
//             else {std::println("Account Creation Failed: Username already exists.");
//                  logReq.status = LogResult::FAILURE;
//                 }
//             break;
//         } // Bracket placed safely after the break

//         case LogCmd::LOGOUT:
//             std::println("{0} is Logging out...", logReq.logCredentials.username);
//             break;

//              case LogCmd::DEL: {
//             std::println("Processing deletion request for: {}", logReq.logCredentials.username);
            
//             // 1. Safe Lookup: Find the account in the map without using operator[]
//             auto it = accountTable.find(logReq.uuid);
            
//             if (it == accountTable.end()) {
//                 std::println("Invalid Deletion Request: Account ID does not exist.");
//                 logReq.status = LogResult::FAILURE;
//                 break;
//             }

//             // 2. Extra Validation: Ensure the requested username matches the token data
//             if (it->second.loginCred.username != logReq.logCredentials.username) {
//                 std::println("Security Violation: Username mismatch for target UUID.");
//                 logReq.status = LogResult::FAILURE;
//                 break;
//             }

//             // 3. Optional Confirmation Step
//             char promptReply;
//             std::println("Deleting Account:\t{}", it->second.loginCred.username);
//             std::println("Enter Y to continue deletion or any other key to cancel request...");
//             std::cin >> promptReply;
            
//             if (promptReply == 'y' || promptReply == 'Y') {
//                 // 4. Save a historical copy into your tombstone map
//                 tombstonedAccountsTable.emplace(it->first, it->second);
                
//                 // 5. In-place data redaction 
//                 it->second.loginCred.password = "TOMBSTONE";
//                 it->second.active = false;
                
//                 // 6. Clean up secondary cache layers so the username can be registered again
//                 nameToUUID.erase(it->second.loginCred.username);
                
//                 logReq.status = LogResult::SUCCESS;
//                 std::println("Deletion Completed Successfully.\n");
//             } else {
//                 std::println("Deletion cancelled by administrative prompt.\n");
//                 logReq.status = LogResult::FAILURE;
//             }
//             break;
//         }
//         default: 
//             std::cerr << "Log Command Switch Error";
//     }
// }
// // Handles a login command placeholder for the server-side command dispatcher.
// void handleLogin() {

//     return;
//     };
// // Handles a data-processing command placeholder for the server-side dispatcher.
// void handleData() { std::cout << "Processing data...\n"; };

// //TODO: Need to find alternative to raw function pointers, maybe std::function or lambdas for more flexibility and type safety.
// static const std::unordered_map<std::string, void(*)()> functionTable = {
//     //TODO: build function map
//     {"login", handleLogin},
//     {"data", handleData}
// };

// // Looks up a command name in the function table and invokes the matching handler.
// void dispatch(const std::string& cmd) {

//     //If the map is empty then early return
//     if (!functionTable.count(cmd)) return;
    
//     //find the command in the map
//     auto it = functionTable.find(cmd);
    
//     //if the command exists then it will not point to the end of the map
//     if (it != functionTable.end()) {
//         it->second(); // Call the associated function 
//     }
// };
// /*
// Performance: std::unordered_map uses a hash table (often with closed addressing) to provide faster lookup than std::map.

// Initialization: Function maps are often declared as static const or within a singleton to ensure they are initialized once at startup.

// Error Handling: When a key is not found, unordered_map::find() should be used instead of operator[] to avoid inserting empty elements into the map. */