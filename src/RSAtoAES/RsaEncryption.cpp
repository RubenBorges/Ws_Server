#include "RsaEncryption.hpp"
#include <iostream>

namespace crpyt {

std::optional<std::vector<unsigned char>> RsaEncryption(std::filesystem::path pubkey, std::string secretMessage) {
    FILE* pubKeyFile = fopen(pubkey.c_str(), "r");
    if (!pubKeyFile) {
        std::cerr << "Failed to open public key file." << std::endl;
        return std::nullopt;
    }
    std::cout << "Public Key file opened successfully: " << pubkey << std::endl;
    EVP_PKEY* publicKey = PEM_read_PUBKEY(pubKeyFile, NULL, NULL, NULL);
    fclose(pubKeyFile);

    if (!publicKey) {
    std::cerr << "Failed to read public key. OpenSSL Error Details:" << std::endl;
    ERR_print_errors_fp(stderr); // <--- Add this line!
    return std::nullopt;
    }

    // 2. Prepare the data to encrypt (e.g., your 256-bit AES Key)
    const unsigned char* plainText = reinterpret_cast<const unsigned char*>(secretMessage.c_str());
    size_t plainTextLen = secretMessage.length();

    // 3. Create the Encryption Context
    EVP_PKEY_CTX* ctx = EVP_PKEY_CTX_new(publicKey, NULL);
    if (!ctx) {
        std::cerr << "Failed to create context." << std::endl;
        EVP_PKEY_free(publicKey);
        return std::nullopt;
    }

    // 4. Initialize the encryption operation
    if (EVP_PKEY_encrypt_init(ctx) <= 0) {
        std::cerr << "Initialization failed." << std::endl;
        EVP_PKEY_CTX_free(ctx);
        EVP_PKEY_free(publicKey);
        return std::nullopt;
    }

    // 5. Configure OAEP Padding and SHA-256
    if (EVP_PKEY_CTX_set_rsa_padding(ctx, RSA_PKCS1_OAEP_PADDING) <= 0 ||
        EVP_PKEY_CTX_set_rsa_oaep_md(ctx, EVP_sha256()) <= 0 ||
        EVP_PKEY_CTX_set_rsa_mgf1_md(ctx, EVP_sha256()) <= 0) {
        std::cerr << "Failed to set secure OAEP padding configurations." << std::endl;
        EVP_PKEY_CTX_free(ctx);
        EVP_PKEY_free(publicKey);
        return std::nullopt;
    }

    // 6. Step 1 of Encryption: Determine the required output buffer size
    size_t cipherTextLen = 0;
    if (EVP_PKEY_encrypt(ctx, NULL, &cipherTextLen, plainText, plainTextLen) <= 0) {
        std::cerr << "Failed to determine buffer size." << std::endl;
        EVP_PKEY_CTX_free(ctx);
        EVP_PKEY_free(publicKey);
        return std::nullopt;
    }

    // Allocate memory for the encrypted bytes based on the size found
    std::vector<unsigned char> cipherText(cipherTextLen);

    // 7. Step 2 of Encryption: Perform the actual mathematical operation
    if (EVP_PKEY_encrypt(ctx, cipherText.data(), &cipherTextLen, plainText, plainTextLen) <= 0) {
        std::cerr << "Encryption failed." << std::endl;
        EVP_PKEY_CTX_free(ctx);
        EVP_PKEY_free(publicKey);
        return std::nullopt;
    }   
    std::cout << "Successfully encrypted! Output size: " << cipherTextLen << " bytes." << std::endl;
    EVP_PKEY_CTX_free(ctx);
    EVP_PKEY_free(publicKey);

    return cipherText;
}
}