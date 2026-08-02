#include <iostream>
#include <vector>
#include <stdexcept>
#include <openssl/evp.h>
#include <openssl/rand.h>

constexpr int AES_256_KEY_SIZE = 32;
constexpr int AES_GCM_IV_SIZE = 12;
constexpr int AES_GCM_TAG_SIZE = 16;

// Helper function to handle OpenSSL errors cleanly
void handleErrors() {
    throw std::runtime_error("OpenSSL cryptographic operation failed.");
}

// Encrypts plaintext and appends the authentication tag to the output buffer
std::vector<uint8_t> aes_gcm_encrypt(const std::vector<uint8_t>& plaintext, 
                                     const std::vector<uint8_t>& key, 
                                     const std::vector<uint8_t>& iv) {
    EVP_CIPHER_CTX* ctx = EVP_CIPHER_CTX_new();
    if (!ctx) handleErrors();

    // Initialize encryption operation with AES-256-GCM
    if (1 != EVP_EncryptInit_ex(ctx, EVP_aes_256_gcm(), nullptr, nullptr, nullptr)) {
        EVP_CIPHER_CTX_free(ctx);
        handleErrors();
    }

    // Set custom IV length if it deviates from the default 12 bytes
    if (1 != EVP_CIPHER_CTX_ctrl(ctx, EVP_CTRL_GCM_SET_IVLEN, iv.size(), nullptr)) {
        EVP_CIPHER_CTX_free(ctx);
        handleErrors();
    }

    // Initialize key and IV
    if (1 != EVP_EncryptInit_ex(ctx, nullptr, nullptr, key.data(), iv.data())) {
        EVP_CIPHER_CTX_free(ctx);
        handleErrors();
    }

    std::vector<uint8_t> ciphertext(plaintext.size());
    int len = 0;
    int ciphertext_len = 0;

    // Provide the message to be encrypted
    if (1 != EVP_EncryptUpdate(ctx, ciphertext.data(), &len, plaintext.data(), plaintext.size())) {
        EVP_CIPHER_CTX_free(ctx);
        handleErrors();
    }
    ciphertext_len = len;

    // Finalize encryption (GCM does not pad, but this compiles the internal state)
    if (1 != EVP_EncryptFinal_ex(ctx, ciphertext.data() + len, &len)) {
        EVP_CIPHER_CTX_free(ctx);
        handleErrors();
    }
    ciphertext_len += len;

    // Extract the authentication tag
    std::vector<uint8_t> tag(AES_GCM_TAG_SIZE);
    if (1 != EVP_CIPHER_CTX_ctrl(ctx, EVP_CTRL_GCM_GET_TAG, AES_GCM_TAG_SIZE, tag.data())) {
        EVP_CIPHER_CTX_free(ctx);
        handleErrors();
    }

    EVP_CIPHER_CTX_free(ctx);

    // Append the tag directly to ciphertext for transit convenience
    ciphertext.insert(ciphertext.end(), tag.begin(), tag.end());
    return ciphertext;
}

// Decrypts ciphertext and verifies integrity using the trailing authentication tag
std::vector<uint8_t> aes_gcm_decrypt(const std::vector<uint8_t>& ciphertext_with_tag, 
                                     const std::vector<uint8_t>& key, 
                                     const std::vector<uint8_t>& iv) {
    if (ciphertext_with_tag.size() < AES_GCM_TAG_SIZE) {
        throw std::runtime_error("Ciphertext payload too short.");
    }

    // Split cipher text and authentication tag
    size_t ciphertext_len = ciphertext_with_tag.size() - AES_GCM_TAG_SIZE;
    const uint8_t* ciphertext_data = ciphertext_with_tag.data();
    const uint8_t* tag_data = ciphertext_with_tag.data() + ciphertext_len;

    EVP_CIPHER_CTX* ctx = EVP_CIPHER_CTX_new();
    if (!ctx) handleErrors();

    // Initialize decryption operation
    if (1 != EVP_DecryptInit_ex(ctx, EVP_aes_256_gcm(), nullptr, nullptr, nullptr)) {
        EVP_CIPHER_CTX_free(ctx);
        handleErrors();
    }

    if (1 != EVP_CIPHER_CTX_ctrl(ctx, EVP_CTRL_GCM_SET_IVLEN, iv.size(), nullptr)) {
        EVP_CIPHER_CTX_free(ctx);
        handleErrors();
    }

    if (1 != EVP_DecryptInit_ex(ctx, nullptr, nullptr, key.data(), iv.data())) {
        EVP_CIPHER_CTX_free(ctx);
        handleErrors();
    }

    std::vector<uint8_t> plaintext(ciphertext_len);
    int len = 0;
    int plaintext_len = 0;

    // Provide the message to be decrypted
    if (1 != EVP_DecryptUpdate(ctx, plaintext.data(), &len, ciphertext_data, ciphertext_len)) {
        EVP_CIPHER_CTX_free(ctx);
        handleErrors();
    }
    plaintext_len = len;

    // Set the expected authentication tag value
    if (1 != EVP_CIPHER_CTX_ctrl(ctx, EVP_CTRL_GCM_SET_TAG, AES_GCM_TAG_SIZE, const_cast<uint8_t*>(tag_data))) {
        EVP_CIPHER_CTX_free(ctx);
        handleErrors();
    }

    // Finalize decryption. A return value of 1 means authentication succeeded.
    int ret = EVP_DecryptFinal_ex(ctx, plaintext.data() + len, &len);
    EVP_CIPHER_CTX_free(ctx);

    if (ret > 0) {
        plaintext_len += len;
        plaintext.resize(plaintext_len);
        return plaintext;
    } else {
        // Integrity check failed: data was modified or key/IV is wrong!
        throw std::runtime_error("Decryption integrity check failed. Data is corrupted or tampered.");
    }
}

int main() {
    // 1. Prepare secret key (Must be securely exchanged beforehand, e.g., via Diffie-Hellman)
//---THE RSA ENCRYPTION AND DECRYPTION PART IS HANDLED IN RsaEncryption.cpp and RsaDecryption.cpp---
// ---- it must occur NOW!-----


    std::vector<uint8_t> secret_key(AES_256_KEY_SIZE, 0x42); // Dummy key value

    // 2. Generate a random IV for this specific message transmission
    std::vector<uint8_t> iv(AES_GCM_IV_SIZE);
    if (1 != RAND_bytes(iv.data(), iv.size())) {
        std::cerr << "Failed to generate random IV." << std::endl;
        return 1;
    }

    // 3. Define communications payload
    std::string secret_message = "Hello! Secure communication established.";
    std::vector<uint8_t> plaintext(secret_message.begin(), secret_message.end());

    try {
        // Encrypt message for sending
        std::vector<uint8_t> package = aes_gcm_encrypt(plaintext, secret_key, iv);
        std::cout << "Encrypted packet size (Ciphertext + Tag): " << package.size() << " bytes.\n";

        // Decrypt message upon receipt
        std::vector<uint8_t> decrypted = aes_gcm_decrypt(package, secret_key, iv);
        std::string recovered_message(decrypted.begin(), decrypted.end());
        
        std::cout << "Decrypted message: " << recovered_message << std::endl;
    } 
    catch (const std::exception& e) {
        std::cerr << "Error: " << e.what() << std::endl;
        return 1;
    }

    return 0;
}
