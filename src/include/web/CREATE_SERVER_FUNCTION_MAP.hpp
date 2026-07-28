//NEED TO CREATE A FUNCTION MAP FOR THE SERVER TO CALL CLIENT FUNCTIONS, AND VICE VERSA, WITHOUT TIGHT COUPLING. THIS WILL ALLOW US TO EASILY ADD NEW FUNCTIONALITY WITHOUT MODIFYING THE CORE SERVER/CLIENT LOGIC.
#include <iostream>
#include <string>
#include <unordered_map>

void handleLogin() { std::cout << "Logging in...\n"; };
void handleData() { std::cout << "Processing data...\n"; };

//TODO: Need to find alternative to raw function pointers, maybe std::function or lambdas for more flexibility and type safety.
static const std::unordered_map<std::string, void(*)()> functionTable = {
    //TODO: build function map
    {"login", handleLogin},
    {"data", handleData}
};

void dispatch(const std::string& cmd) {

    //If the map is empty then early return
    if (!functionTable.count(cmd)) return;
    
    //find the command in the map
    auto it = functionTable.find(cmd);
    
    //if the command exists then it will not point to the end of the map
    if (it != functionTable.end()) {
        it->second(); // Call the associated function 
    }
};

Performance: std::unordered_map uses a hash table (often with closed addressing) to provide faster lookup than std::map.

Initialization: Function maps are often declared as static const or within a singleton to ensure they are initialized once at startup.

Error Handling: When a key is not found, unordered_map::find() should be used instead of operator[] to avoid inserting empty elements into the map. 0000