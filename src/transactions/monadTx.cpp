#include <iostream>
#include <optional>

struct transaction {
    int id;
    bool valid       = false;
    bool transformed = false;
};

  // --- Monadic API Functions ---
  // Sources: Return optional to represent "Found" or "Not Found"
std::optional<transaction> fetchFrom_Cache(const transaction& tx) {
      // Logic: If not in cache, return nullopt to trigger or_else
    return std::nullopt; 
}

std::optional<transaction> fetchFrom_Server(const transaction& tx) {
    std::cout << "Fetching from Server..." << std::endl;
    return tx;  // Found it
}

// Operators: and_then expects these to return optional
std::optional<transaction> validate_transaction(const transaction& tx) {
    transaction result = tx;
    result.valid = true;
    std::cout << "Transaction Validated." << std::endl;
    return result;
}

std::optional<transaction> Transform_transaction(const transaction& tx) {
    transaction result = tx;
    result.transformed = true;
    std::cout << "Transaction Transformed." << std::endl;
    return result;
}

// Sinks: Usually the end of the chain
std::optional<transaction> passTo_Sink(const transaction& tx) {
    std::cout << "Transaction pushed to sink." << std::endl;
    return tx;
}

std::optional<transaction> process_transaction(const transaction& tx) {
    return fetchFrom_Cache(tx)
        .or_else([&]() { return fetchFrom_Server(tx); }) // Fallback if cache fails
        .and_then(validate_transaction)                  // Only if found
        .and_then(Transform_transaction)                 // Only if valid
        .and_then(passTo_Sink);                          // Only if transformed
}

int main() {
    transaction myTx{101};
    
    auto result = process_transaction(myTx);

    if (result) {
        std::cout << "Flow complete for Tx: " << result->id << std::endl;
    } else {
        std::cout << "Flow aborted: A step in the chain failed." << std::endl;
    }
}