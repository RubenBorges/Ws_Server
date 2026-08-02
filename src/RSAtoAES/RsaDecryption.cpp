#include "RsaDecryption.hpp"
#include <fstream>
#include <iostream>
#include <optional>
#include <string>

namespace crypt  {

std::optional<std::string> RsaDecryption(std::filesystem::path privatekey, std::vector<unsigned char>& cipherText) {

    // 1. Read the PEM Private Key file
    FILE* privKeyFile = fopen(privatekey.string().c_str(), "r");
    if (!privKeyFile) {
        std::cerr << "Failed to open private key file." << std::endl;
        return std::nullopt;
    }

    EVP_PKEY* privateKey = PEM_read_PrivateKey(privKeyFile, NULL, NULL, NULL);
    fclose(privKeyFile);

    if (!privateKey) {
        std::cerr << "Failed to read private key." << std::endl;
        return std::nullopt;
    }

    // 2. Mock ciphertext input (In a real app, read this from your network socket or file)
    // For this example, assume 'cipherText' contains the bytes received from the sender.
    // std::ifstream inFile(encrypted_file.string().c_str(), std::ios::binary);
    // if (!inFile) {
    //     std::cerr << "Failed to open encrypted input file." << std::endl;
    //     EVP_PKEY_free(privateKey);
    //     return std::nullopt;
    // }

    // Read the encrypted data from the file
    // std::vector<unsigned char> cipherText((std::istreambuf_iterator<char>(inFile)), std::istreambuf_iterator<char>());
    size_t cipherTextLen = cipherText.size();
    // inFile.close();

    if (cipherTextLen == 0) {
        std::cerr << "Ciphertext buffer is empty. Provide real encrypted data." << std::endl;
        EVP_PKEY_free(privateKey);
        return std::nullopt;
    }

    // 3. Create the Decryption Context
    EVP_PKEY_CTX* ctx = EVP_PKEY_CTX_new(privateKey, NULL);
    if (!ctx) {
        std::cerr << "Failed to create context." << std::endl;
        EVP_PKEY_free(privateKey);
        return std::nullopt;
    }

    // 4. Initialize the decryption operation
    if (EVP_PKEY_decrypt_init(ctx) <= 0) {
        std::cerr << "Decryption initialization failed." << std::endl;
        EVP_PKEY_CTX_free(ctx);
        EVP_PKEY_free(privateKey);
        return std::nullopt;
    }

    // 5. Configure OAEP Padding and SHA-256 (Must strictly match the encryption side)
    if (EVP_PKEY_CTX_set_rsa_padding(ctx, RSA_PKCS1_OAEP_PADDING) <= 0 ||
        EVP_PKEY_CTX_set_rsa_oaep_md(ctx, EVP_sha256()) <= 0 ||
        EVP_PKEY_CTX_set_rsa_mgf1_md(ctx, EVP_sha256()) <= 0) {
        std::cerr << "Failed to set secure OAEP padding configurations." << std::endl;
        EVP_PKEY_CTX_free(ctx);
        EVP_PKEY_free(privateKey);
        return std::nullopt;
    }

    // 6. Step 1 of Decryption: Determine the required output buffer size
    size_t plainTextLen = 0;
    if (EVP_PKEY_decrypt(ctx, NULL, &plainTextLen, cipherText.data(), cipherTextLen) <= 0) {
        std::cerr << "Failed to determine plaintext buffer size." << std::endl;
        EVP_PKEY_CTX_free(ctx);
        EVP_PKEY_free(privateKey);
        return std::nullopt;
    }

    // Allocate memory for the decrypted message
    std::vector<unsigned char> plainText(plainTextLen);

    // 7. Step 2 of Decryption: Perform the actual mathematical operation and strip padding
    if (EVP_PKEY_decrypt(ctx, plainText.data(), &plainTextLen, cipherText.data(), cipherTextLen) <= 0) {
        std::cerr << "Decryption failed! Data may be corrupted or key is incorrect." << std::endl;
        EVP_PKEY_CTX_free(ctx);
        EVP_PKEY_free(privateKey);
        return std::nullopt;
    }

    // 8. Convert the plain text bytes back to a readable C++ string
    std::string recoveredMessage(reinterpret_cast<char*>(plainText.data()), plainTextLen);
    std::cout << "Successfully Decrypted!" << std::endl;
    std::cout << "Recovered Message: " << recoveredMessage << std::endl;
    // Clean up memory
    EVP_PKEY_CTX_free(ctx);
    EVP_PKEY_free(privateKey);
    
    return std::make_optional(recoveredMessage);
}

std::optional<std::string> RsaDecryption(std::filesystem::path privatekey, std::filesystem::path encrypted_file) {

    // 1. Read the PEM Private Key file
    FILE* privKeyFile = fopen(privatekey.string().c_str(), "r");
    if (!privKeyFile) {
        std::cerr << "Failed to open private key file." << std::endl;
        return std::nullopt;
    }

    EVP_PKEY* privateKey = PEM_read_PrivateKey(privKeyFile, NULL, NULL, NULL);
    fclose(privKeyFile);

    if (!privateKey) {
        std::cerr << "Failed to read private key." << std::endl;
        return std::nullopt;
    }

    // 2. Mock ciphertext input (In a real app, read this from your network socket or file)
    // For this example, assume 'cipherText' contains the bytes received from the sender.
    std::ifstream inFile(encrypted_file.string().c_str(), std::ios::binary);
    if (!inFile) {
        std::cerr << "Failed to open encrypted input file." << std::endl;
        EVP_PKEY_free(privateKey);
        return std::nullopt;
    }

    // Read the encrypted data from the file
    std::vector<unsigned char> cipherText((std::istreambuf_iterator<char>(inFile)), std::istreambuf_iterator<char>());
    size_t cipherTextLen = cipherText.size();
    inFile.close();

    if (cipherTextLen == 0) {
        std::cerr << "Ciphertext buffer is empty. Provide real encrypted data." << std::endl;
        EVP_PKEY_free(privateKey);
        return std::nullopt;
    }

    // 3. Create the Decryption Context
    EVP_PKEY_CTX* ctx = EVP_PKEY_CTX_new(privateKey, NULL);
    if (!ctx) {
        std::cerr << "Failed to create context." << std::endl;
        EVP_PKEY_free(privateKey);
        return std::nullopt;
    }

    // 4. Initialize the decryption operation
    if (EVP_PKEY_decrypt_init(ctx) <= 0) {
        std::cerr << "Decryption initialization failed." << std::endl;
        EVP_PKEY_CTX_free(ctx);
        EVP_PKEY_free(privateKey);
        return std::nullopt;
    }

    // 5. Configure OAEP Padding and SHA-256 (Must strictly match the encryption side)
    if (EVP_PKEY_CTX_set_rsa_padding(ctx, RSA_PKCS1_OAEP_PADDING) <= 0 ||
        EVP_PKEY_CTX_set_rsa_oaep_md(ctx, EVP_sha256()) <= 0 ||
        EVP_PKEY_CTX_set_rsa_mgf1_md(ctx, EVP_sha256()) <= 0) {
        std::cerr << "Failed to set secure OAEP padding configurations." << std::endl;
        EVP_PKEY_CTX_free(ctx);
        EVP_PKEY_free(privateKey);
        return std::nullopt;
    }

    // 6. Step 1 of Decryption: Determine the required output buffer size
    size_t plainTextLen = 0;
    if (EVP_PKEY_decrypt(ctx, NULL, &plainTextLen, cipherText.data(), cipherTextLen) <= 0) {
        std::cerr << "Failed to determine plaintext buffer size." << std::endl;
        EVP_PKEY_CTX_free(ctx);
        EVP_PKEY_free(privateKey);
        return std::nullopt;
    }

    // Allocate memory for the decrypted message
    std::vector<unsigned char> plainText(plainTextLen);

    // 7. Step 2 of Decryption: Perform the actual mathematical operation and strip padding
    if (EVP_PKEY_decrypt(ctx, plainText.data(), &plainTextLen, cipherText.data(), cipherTextLen) <= 0) {
        std::cerr << "Decryption failed! Data may be corrupted or key is incorrect." << std::endl;
        EVP_PKEY_CTX_free(ctx);
        EVP_PKEY_free(privateKey);
        return std::nullopt;
    }

    // 8. Convert the plain text bytes back to a readable C++ string
    std::string recoveredMessage(reinterpret_cast<char*>(plainText.data()), plainTextLen);
    
    std::cout << "Successfully Decrypted!" << std::endl;
    std::cout << "Recovered Message: " << recoveredMessage << std::endl;

    // Clean up memory
    EVP_PKEY_CTX_free(ctx);
    EVP_PKEY_free(privateKey);

    return std::make_optional(recoveredMessage);
}   
}
