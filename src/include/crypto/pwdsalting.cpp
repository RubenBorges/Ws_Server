#include <iostream>
#include <string>
#include <vector>
#include <iomanip>
#include <stdexcept>
#include <memory>
#include <openssl/evp.h>
#include <openssl/rand.h>
#include <openssl/kdf.h>
#include <openssl/core_names.h>
#include <openssl/params.h>

// Cryptographic Sizing Constants
constexpr size_t AES_KEY_SIZE = 32;       // 256-bit key
constexpr size_t GCM_IV_SIZE = 12;        // 96-bit IV (Standard for GCM)
constexpr size_t GCM_TAG_SIZE = 16;       // 128-bit authentication tag
constexpr size_t SALT_SIZE = 16;          // 128-bit salt
constexpr int PBKDF2_ITERATIONS = 600000; // OWASP recommendation for SHA-256

// Context wrappers using RAII for automatic memory cleanup
using CipherCtxPtr = std::unique_ptr<EVP_CIPHER_CTX, void(*)(EVP_CIPHER_CTX*)>;
using KdfCtxPtr = std::unique_ptr<EVP_KDF_CTX, void(*)(EVP_KDF_CTX*)>;

// Container package representing our encrypted payload data
struct SecurePackage {
    std::vector<uint8_t> ciphertext;
    std::vector<uint8_t> iv;
    std::vector<uint8_t> tag;
    std::vector<uint8_t> salt;
};

// Derived Key Helper: Generates or uses a salt to extract an AES key from a password string
std::vector<uint8_t> derive_key(const std::string& password, std::vector<uint8_t>& salt, bool generating_new_salt) {
    if (generating_new_salt) {
        salt.resize(SALT_SIZE);
        if (RAND_bytes(salt.data(), SALT_SIZE) != 1) {
            throw std::runtime_error("CSPRNG failed to generate a random salt");
        }
    } else if (salt.size() != SALT_SIZE) {
        throw std::invalid_argument("Provided validation salt size is incorrect");
    }

    std::vector<uint8_t> key(AES_KEY_SIZE);

    std::unique_ptr<EVP_KDF, void(*)(EVP_KDF*)> kdf(EVP_KDF_fetch(nullptr, "PBKDF2", nullptr), EVP_KDF_free);
    if (!kdf) throw std::runtime_error("Failed to fetch PBKDF2 provider framework");

    KdfCtxPtr kdf_ctx(EVP_KDF_CTX_new(kdf.get()), EVP_KDF_CTX_free);
    if (!kdf_ctx) throw std::runtime_error("Failed to initialize PBKDF2 context architecture");

    char digest_name[] = "SHA256";
    int iterations = PBKDF2_ITERATIONS;
    OSSL_PARAM params[5];
    
    params[0] = OSSL_PARAM_construct_utf8_string(OSSL_KDF_PARAM_DIGEST, digest_name, 0);
    params[1] = OSSL_PARAM_construct_octet_string(OSSL_KDF_PARAM_PASSWORD, (void*)password.c_str(), password.length());
    params[2] = OSSL_PARAM_construct_octet_string(OSSL_KDF_PARAM_SALT, salt.data(), salt.size());
    params[3] = OSSL_PARAM_construct_int(OSSL_KDF_PARAM_ITER, &iterations);
    params[4] = OSSL_PARAM_construct_end();

    if (EVP_KDF_derive(kdf_ctx.get(), key.data(), key.size(), params) != 1) {
        throw std::runtime_error("Key derivation sequence failed internally");
    }

    return key;
}

// Industry Standard AES-256-GCM Encryption Function
SecurePackage encrypt_payload(const std::string& plaintext, const std::string& password) {
    SecurePackage package;
    
    // 1. Derive an ephemeral AES-256 Key and generate a fresh random Salt
    std::vector<uint8_t> derived_key = derive_key(password, package.salt, true);

    // 2. Prepare storage containers and generate a cryptographically secure random IV
    package.iv.resize(GCM_IV_SIZE);
    package.ciphertext.resize(plaintext.size());
    if (RAND_bytes(package.iv.data(), GCM_IV_SIZE) != 1) {
        throw std::runtime_error("CSPRNG failed to generate initialization vector");
    }

    // 3. Initialize cipher stream context
    CipherCtxPtr ctx(EVP_CIPHER_CTX_new(), EVP_CIPHER_CTX_free);
    if (!ctx) throw std::runtime_error("Cipher context instantiation failed");

    if (EVP_EncryptInit_ex(ctx.get(), EVP_aes_256_gcm(), nullptr, nullptr, nullptr) != 1) {
        throw std::runtime_error("Failed to match GCM engine mapping initialization");
    }

    if (EVP_CIPHER_CTX_ctrl(ctx.get(), EVP_CTRL_AEAD_SET_IVLEN, GCM_IV_SIZE, nullptr) != 1) {
        throw std::runtime_error("Failed setting structural GCM IV sizing layout");
    }

    if (EVP_EncryptInit_ex(ctx.get(), nullptr, nullptr, derived_key.data(), package.iv.data()) != 1) {
        throw std::runtime_error("Binding derived key framework data blocks failed");
    }

    // 4. Processing plaintext payload array block segments
    int len = 0;
    if (EVP_EncryptUpdate(ctx.get(), package.ciphertext.data(), &len, 
                           reinterpret_cast<const uint8_t*>(plaintext.data()), plaintext.size()) != 1) {
        throw std::runtime_error("Core text stream encryption stage crashed");
    }
    int total_ciphertext_len = len;

    // 5. Finalizing structural padding alignments
    if (EVP_EncryptFinal_ex(ctx.get(), package.ciphertext.data() + len, &len) != 1) {
        throw std::runtime_error("Finalizing processing array block sequence crashed");
    }
    total_ciphertext_len += len;
    package.ciphertext.resize(total_ciphertext_len);

    // 6. Extract the cryptographic authentication tag
    package.tag.resize(GCM_TAG_SIZE);
    if (EVP_CIPHER_CTX_ctrl(ctx.get(), EVP_CTRL_AEAD_GET_TAG, GCM_TAG_SIZE, package.tag.data()) != 1) {
        throw std::runtime_error("Authentication verification tag capture crashed");
    }

    return package;
}

// Industry Standard AES-256-GCM Decryption Function
std::string decrypt_payload(const SecurePackage& package, const std::string& password) {
    if (package.salt.size() != SALT_SIZE || package.iv.size() != GCM_IV_SIZE || package.tag.size() != GCM_TAG_SIZE) {
        throw std::invalid_argument("Malformed secure payload package framing lengths");
    }

    // 1. Rebuild the exact same AES key using the password and the SAVED salt
    std::vector<uint8_t> saved_salt = package.salt; 
    std::vector<uint8_t> derived_key = derive_key(password, saved_salt, false);

    std::vector<uint8_t> plaintext_buffer(package.ciphertext.size());

    // 2. Initialize cipher decryption structure mappings
    CipherCtxPtr ctx(EVP_CIPHER_CTX_new(), EVP_CIPHER_CTX_free);
    if (!ctx) throw std::runtime_error("Cipher decryption context configuration crashed");

    if (EVP_DecryptInit_ex(ctx.get(), EVP_aes_256_gcm(), nullptr, nullptr, nullptr) != 1) {
        throw std::runtime_error("Failed setting GCM engine inversion mapping");
    }

    if (EVP_CIPHER_CTX_ctrl(ctx.get(), EVP_CTRL_AEAD_SET_IVLEN, GCM_IV_SIZE, nullptr) != 1) {
        throw std::runtime_error("Failed setting structural decryption IV parameters");
    }

    if (EVP_DecryptInit_ex(ctx.get(), nullptr, nullptr, derived_key.data(), package.iv.data()) != 1) {
        throw std::runtime_error("Binding inversion key parameters block data failed");
    }

    // 3. Process encrypted stream back down into plaintext segments
    int len = 0;
    if (EVP_DecryptUpdate(ctx.get(), plaintext_buffer.data(), &len, package.ciphertext.data(), package.ciphertext.size()) != 1) {
        throw std::runtime_error("Core stream payload decryption processing failed");
    }
    int total_plaintext_len = len;

    // 4. Provide the expected authentication validation tag to OpenSSL *before* finalizing
    if (EVP_CIPHER_CTX_ctrl(ctx.get(), EVP_CTRL_AEAD_SET_TAG, GCM_TAG_SIZE, const_cast<uint8_t*>(package.tag.data())) != 1) {
        throw std::runtime_error("Setting targeted data verification tag metrics failed");
    }

    // 5. Finalize processing and execute the cryptographic authentication check
    // If the data was altered, or if someone typed a wrong password, this step fails safely.
    if (EVP_DecryptFinal_ex(ctx.get(), plaintext_buffer.data() + len, &len) <= 0) {
        throw std::runtime_error("AUTHENTICATION REJECTION: Incorrect password or modified ciphertext data payload detected!");
    }
    total_plaintext_len += len;
    plaintext_buffer.resize(total_plaintext_len);

    return std::string(plaintext_buffer.begin(), plaintext_buffer.end());
}

// Hex printing utility
void display_hex(const std::string& description, const std::vector<uint8_t>& vector_data) {
    std::cout << description << ": ";
    for (uint8_t segment_byte : vector_data) {
        std::cout << std::hex << std::setw(2) << std::setfill('0') << static_cast<int>(segment_byte);
    }
    std::cout << std::dec << "\n";
}

int main() {
    try {
        std::string secret_phrase = "Highly Classified Configuration String Content";
        std::string user_password = "CorrectSecurePassword789!";

        std::cout << "Original Payload: " << secret_phrase << "\n\n";

        // --- ENCRYPTION PIPELINE ---
        SecurePackage encrypted_bundle = encrypt_payload(secret_phrase, user_password);
        std::cout << "--- [Encryption Complete] ---\n";
        display_hex("Stored Salt (Public)      ", encrypted_bundle.salt);
        display_hex("Initialization Vector (IV)", encrypted_bundle.iv);
        display_hex("Authentication Tag        ", encrypted_bundle.tag);
        display_hex("Encrypted Ciphertext Raw  ", encrypted_bundle.ciphertext);
        std::cout << "\n";

        // --- DECRYPTION PIPELINE (SUCCESS CASE) ---
        std::cout << "--- [Attempting Decryption with Correct Password] ---\n";
        std::string recovery_text = decrypt_payload(encrypted_bundle, user_password);
        std::cout << "Decrypted String Output: " << recovery_text << "\n\n";

        // --- DECRYPTION PIPELINE (FAILURE CONTEXT VALIDATION) ---
        std::cout << "--- [Attempting Decryption with Bad Password] ---\n";
        std::string bad_password_attempt = "WrongPassword123!";
        // This will deliberately trip the internal error catch engine tracking system
        std::string failed_output = decrypt_payload(encrypted_bundle, bad_password_attempt);
        std::cout << "Decrypted String Output: " << failed_output << "\n";

    } catch (const std::exception& captured_error) {
        std::cerr << "\nIntercepted Runtime Error: " << captured_error.what() << "\n";
    }
    return 0;
}
