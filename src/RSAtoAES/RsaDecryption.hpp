#include <openssl/evp.h>
#include <openssl/pem.h>
#include <openssl/err.h>
#include <filesystem>
#include <vector>
namespace crypt {
std::optional<std::string>  RsaDecryption(std::filesystem::path privatekey,std::vector<unsigned char>& cipherText );
}