#pragma once // Crucial: Prevents recursive header inclusion bugs

#include <cstdlib>
#include <jwt-cpp/jwt.h>
#include <string>
#include <utility>
#include <vector>
#include <print>
#include <span>
#include <algorithm>
#include <chrono>
#include <boost/uuid/uuid.hpp>
#include <boost/uuid/uuid_io.hpp>
#include <boost/uuid/uuid_generators.hpp>
#include <boost/random/random_device.hpp>

using uuid_T = boost::uuids::uuid;
using sessionToken_t = std::string;

enum class LogCmd : int { LOGIN, CREATE, LOGOUT, DEL };
enum class LogResult : int { SUCCESS = 0, FAILURE = 1, PENDING = 2 };

struct loginCredentials {
    std::string username;
    std::string password;
    friend bool operator==(const loginCredentials& lhs, const loginCredentials& rhs) {
        return lhs.username == rhs.username && lhs.password == rhs.password;
    }
};
    
// User Account struct containing Login Info and UUID
struct loginRequest {
    LogCmd cmd;
    LogResult status{LogResult::PENDING};
    loginCredentials logCredentials;
    uuid_T uuid;
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
    Account* account{nullptr};
};

// FIX: Changed 'static' to 'inline' so all CPP files share the exact same instance!
inline loginRequest ThisLoginRequest;
inline session_t session;
    
namespace account {

    // Will test to see if {target} variable T is present in a range<type T> 
    // Returns true if variable is not found in range
    template<typename T>
    bool is_new(std::span<T> range, const T& testTarget) {
        auto it = std::ranges::find_if(range, [&testTarget](const T& id) {
            return id == testTarget;
        });
        return it == range.end();
    }
    
    // Generates a random RFC 4122 version 4 UUID string
    inline boost::uuids::uuid generate_any_uuid() {
        static boost::uuids::random_generator gen; // Static internal means it seeds once
        return gen();
    }

    // FIX: Accepted std::span by VALUE, not by reference. Spans are lightweight views.
    inline boost::uuids::uuid generate_uuid(std::span<boost::uuids::uuid> idTable) {
        boost::uuids::uuid id;
        do {
            id = generate_any_uuid();
        } while (is_new<boost::uuids::uuid>(idTable, id) == false);
        return id;
    }

    inline Account createAccount(loginCredentials logInput, std::vector<boost::uuids::uuid>& uuidTable) {
        std::span<boost::uuids::uuid> uuidSpan = uuidTable;
        Account acc{logInput, generate_uuid(uuidSpan)};
        uuidTable.emplace_back(acc.uuid);
        return acc;
    }

    inline std::string generateSecureSessionToken() {
        static boost::uuids::basic_random_generator<boost::random_device> secureGen;
        boost::uuids::uuid token = secureGen();
        return boost::uuids::to_string(token);
    }

    inline std::string generateJWTSessionToken(const std::string& username, const std::string& uuidStr = generateSecureSessionToken()) {
        auto token = jwt::create()
            .set_issuer("auth_server")
            .set_type("JWS")
            .set_issued_at(std::chrono::system_clock::now())
            .set_expires_at(std::chrono::system_clock::now() + std::chrono::hours(24)) // 24hr expiry
            .set_payload_claim("username", jwt::claim(username))
            .set_payload_claim("uuid", jwt::claim(uuidStr))
            .sign(jwt::algorithm::hs256{"YOUR_SECRET_CRYPTO_KEY"}); // HMAC SHA256 signing

        return token;
    }

    // FIX: Dropped invalid static layout keyword placement context
    inline bool validateLogin(const Account& acc, const loginCredentials& logInput) {
        return acc.loginCred == logInput;
    }
}
