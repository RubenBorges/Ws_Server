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
    //-----CLIENT INITIATES A REQUEST FOR ENCYPTED CHAT COMMUNICATION WITH SERVER-----
    //-----SERVER PRODUCES ASSYMETRIC KEY PAIR AND SENDS ITS PUBLIC KEY TO CLIENT-----
    std::string SecretSymmeticKey = "SecretPassword123";
    auto keyPair = crypt::GenKeyPair();
    std::cout<< "Generated RSA Key Pair:\n";
    std::cout << " > Public Key: " << keyPair->first << "\n";
    std::cout << " > Private Key:  " << keyPair->second << "\n\n";

    //-----CLIENT USES SERVER'S PUBLIC KEY TO ENCRYPT A SYMMETRIC KEY AND SENDS IT BACK TO SERVER-----
    std::vector<unsigned char> encryptedMessage = *crypt::RsaEncryption(keyPair->first, SecretSymmeticKey);
    std::cout << "Encrypted Symmetric Key: ";
    for (const auto& byte : encryptedMessage) {
        std::cout << std::hex << std::setw(2) << std::setfill('0') << static_cast<int>(byte);
    }
    std::cout << std::dec << std::endl;


    //-----SERVER USES HIS PRIVATE KEY TO DECRYPT THE SYMMETRIC KEY SENT BY CLIENT-----
    std::string const SymmetricKey = *crypt::RsaDecryption(keyPair->second, encryptedMessage);

    //-----SERVER AND CLIENT NOW HAVE THE SAME SYMMETRIC KEY AND CAN USE IT FOR EFFICIENT ENCRYPTED COMMUNICATION-----
    std::cout << "Decrypted Symmetric Key: " << SymmetricKey << std::endl;  
    
    return EXIT_SUCCESS;
}