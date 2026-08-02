#include "genkeypair.hpp"
#include <iostream>
#include <optional>

namespace crypt {

 std::optional<std::pair<std::filesystem::path, std::filesystem::path>> 
    generateAndSaveKeyPair(std::filesystem::path privateKeyPath, std::filesystem::path publicKeyPath) {
    EVP_PKEY_CTX* ctx = EVP_PKEY_CTX_new_id(EVP_PKEY_RSA, NULL);
    EVP_PKEY* pkey = NULL;

    if (!ctx) return std::nullopt;

    // 1. Initialize Key Generation Context
    if (EVP_PKEY_keygen_init(ctx) <= 0) {
        EVP_PKEY_CTX_free(ctx);
        return std::nullopt;
    }

    // 2. Set RSA Key Size to 2048 bits
    if (EVP_PKEY_CTX_set_rsa_keygen_bits(ctx, 2048) <= 0) {
        EVP_PKEY_CTX_free(ctx);
        return std::nullopt ;
    }

    // 3. Generate the actual keypair into memory
    if (EVP_PKEY_keygen(ctx, &pkey) <= 0) {
        EVP_PKEY_CTX_free(ctx);
        return std::nullopt;
    }

    // 4. Save the Private Key to "private_key.pem"
    BIO* privBio = BIO_new_file(privateKeyPath.string().c_str(), "w");
    if (privBio) {
        PEM_write_bio_PrivateKey(privBio, pkey, NULL, NULL, 0, NULL, NULL);
        BIO_free(privBio);
    }

    // 5. Save the Public Key to "public_key.pem"
    BIO* pubBio = BIO_new_file(publicKeyPath.string().c_str(), "w");
    if (pubBio) {
        PEM_write_bio_PUBKEY(pubBio, pkey);
        BIO_free(pubBio);
    }

    // Clean up key structures
    EVP_PKEY_free(pkey);
    EVP_PKEY_CTX_free(ctx);
    return std::make_pair(publicKeyPath, privateKeyPath);
}

std::optional<std::pair<std::filesystem::path, std::filesystem::path>> GenKeyPair(std::filesystem::path privateKeyPath , std::filesystem::path publicKeyPath) {
    std::optional<std::pair<std::filesystem::path, std::filesystem::path>> keyPair = generateAndSaveKeyPair(privateKeyPath, publicKeyPath);
    if (keyPair == std::nullopt) {std::cerr << "Key generation failed." << std::endl; return std::nullopt;}
    return keyPair;
}
}