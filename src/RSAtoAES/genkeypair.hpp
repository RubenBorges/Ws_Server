#include <openssl/evp.h>
#include <openssl/pem.h>
#include <openssl/bio.h>
#include <utility>
#include <filesystem>
#include <optional>

namespace crypt {
std::optional<std::pair<std::filesystem::path, std::filesystem::path>> generateAndSaveKeyPair(std::filesystem::path privateKeyPath, std::filesystem::path publicKeyPath );
std::optional<std::pair<std::filesystem::path, std::filesystem::path>> GenKeyPair(std::filesystem::path privateKeyPath = "private_key.pem", std::filesystem::path publicKeyPath = "public_key.pem");
}