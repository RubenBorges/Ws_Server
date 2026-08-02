#include <cstdlib>
#include <iostream>
#include <string>
#include <vector>
#include <iomanip>
#include "genkeypair.hpp"
#include "RsaEncryption.hpp"
#include "RsaDecryption.hpp"


int main (){
//THE ASSYMETRIC->SYMMETRIC ENCRYPTION/DECRYPTION FLOW
    //-----ASSUME ALICE INITIATES A REQUEST FOR ENCYPTED CHAT COMMUNICATION WITH BOB-----
    //-----BOB PRODUCES ASSYMETRIC KEY PAIR AND SENDS PUBLIC KEY TO ALICE-----
    std::string SecretSymmeticKey = "SecretPassword123";
    auto keyPair = crypt::GenKeyPair();
    std::cout<< "Generated RSA Key Pair:\n";
    std::cout << " > Public Key: " << keyPair->first << "\n";
    std::cout << " > Private Key:  " << keyPair->second << "\n\n";

    //-----ALICE USES BOB'S PUBLIC KEY TO ENCRYPT A SYMMETRIC KEY AND SENDS IT BACK TO BOB-----
    std::vector<unsigned char> encryptedMessage = *crypt::RsaEncryption(keyPair->first, SecretSymmeticKey);
    std::cout << "Encrypted Symmetric Key: ";
    for (const auto& byte : encryptedMessage) {
        std::cout << std::hex << std::setw(2) << std::setfill('0') << static_cast<int>(byte);
    }
    std::cout << std::dec << std::endl;


    //-----BOB USES HIS PRIVATE KEY TO DECRYPT THE SYMMETRIC KEY SENT BY ALICE-----
    std::string const SymmetricKey = *crypt::RsaDecryption(keyPair->second, encryptedMessage);

    //-----BOB NOW HAS THE SYMMETRIC KEY AND CAN USE IT FOR FURTHER ENCRYPTED COMMUNICATION-----
    std::cout << "Decrypted Symmetric Key: " << SymmetricKey << std::endl;  
    
    return EXIT_SUCCESS;
}