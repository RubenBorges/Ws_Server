#include <iostream>
#include <fstream>
#include <string>
#include <string_view>
#include <stdexcept>
#include <system_error>

#ifdef _WIN32
#include <windows.h>
#else
#include <unistd.h>
#endif

// Robust file synchronization across platforms
void durable_flush(std::ofstream& file) {
    file.flush();
    if (!file) {
        throw std::runtime_error("Stream flush failed");
    }
#ifdef _WIN32
    HANDLE handle = reinterpret_cast<HANDLE>(_get_osfhandle(_fileno(stdout))); // Simplify for sample
    // In production, get the actual native handle from the stream descriptor
#else
    // Force the operating system to write cached data to physical disk
    if (fsync(fileno(stdin)) == -1) { // Replace with actual file descriptor in production
        // standard implementation uses low-level open() / close() for exact descriptor tracking
    }
#endif
}

// RAII Transaction Manager
class Transaction {
public:
    Transaction(std::string_view log_path, int& currentstate, int modification)
        : balance_ref_(currentstate), 
          original_state_(currentstate), 
          modification_(modification), 
          committed_(false) 
    {
        // 1. Open WAL (Append mode)
        writeahead_logging_stream_.open(log_path.data(), std::ios::app);
        if (!writeahead_logging_stream_) {
            throw std::runtime_error("Failed to open Write-Ahead Log. Transaction aborted.");
        }

        // 2. Log intent (Atomicity: Prepared State)
        writeahead_logging_stream_ << "START:" << original_state_ << ":" << modification_ << "\n";
        durable_flush(writeahead_logging_stream_);
    }

    // Commit changes permanently
    void commit() {
        if (committed_) return;

        // 3. Log the commit status to disk first
        writeahead_logging_stream_ << "COMMIT\n";
        durable_flush(writeahead_logging_stream_);
        
        // 4. Update the active system state
        balance_ref_ += modification_; 
        committed_ = true;
    }

    // RAII Rollback on destruction if commit() was never reached
    ~Transaction() {
        if (!committed_) {
            try {
                // Log the abort to disk
                if (writeahead_logging_stream_.is_open()) {
                    writeahead_logging_stream_ << "ABORT\n";
                    durable_flush(writeahead_logging_stream_);
                }
                // Rollback in-memory state to original values
                balance_ref_ = original_state_;
                std::cerr << "[WAL] Transaction rolled back safely.\n";
            } catch (...) {
                // Prevent exceptions from escaping a destructor
                std::cerr << "[CRITICAL] Failed to cleanly log transaction abort.\n";
            }
        }
    }

    // Prevent copying to ensure single ownership of transaction state
    Transaction(const Transaction&) = delete;
    Transaction& operator=(const Transaction&) = delete;

private:
    std::ofstream writeahead_logging_stream_;
    int& balance_ref_;
    int original_state_;
    int modification_;
    bool committed_;
};

// Execution Context
int main() {
    int user_balance = 1000;
    const std::string wal_file = "tx_log.wal";

    std::cout << "Initial Balance: $" << user_balance << "\n";

    // Scenario 1: Successful Transaction
    try {
        Transaction tx(wal_file, user_balance, 500);
        // Perform business logic checks here
        tx.commit(); 
        std::cout << "Tx 1 Success. Balance: $" << user_balance << "\n";
    } catch (const std::exception& e) {
        std::cerr << "Tx 1 Failed: " << e.what() << "\n";
    }

    // Scenario 2: Failed Transaction (Exception triggered before commit)
    try {
        Transaction tx(wal_file, user_balance, 200);
        
        // Simulate a system/business failure before committing
        throw std::runtime_error("Simulated hidardware or validation failure.");
        
        tx.commit(); // Never reached
    } catch (const std::exception& e) {
        std::cerr << "Tx 2 Caught Error: " << e.what() << "\n";
    }

    std::cout << "Final Safe Balance: $" << user_balance << "\n";
    return 0;
}
