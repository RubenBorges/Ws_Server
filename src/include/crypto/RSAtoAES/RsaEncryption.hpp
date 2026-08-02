#include <openssl/evp.h>
#include <openssl/pem.h>
#include <openssl/err.h>
#include <filesystem>
#include <optional>
#include <vector>
#include <filesystem>
namespace crypt {
std::optional<std::vector<unsigned char>> RsaEncryption(std::filesystem::path pubkey, std::string secretMessage = "Hello, this is a secret message that needs to be encrypted using RSA with OAEP padding and SHA-256!");
}