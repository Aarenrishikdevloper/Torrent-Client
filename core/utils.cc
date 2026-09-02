# include "crypto++/sha.h"
#include "../includes/utils.hpp"
#include "crypto++/hex.h"
 std::string sha1(const std::string& input) {
    CryptoPP::SHA1 sha1;
    CryptoPP::byte digest[CryptoPP::SHA1::DIGESTSIZE];
     sha1.CalculateDigest(digest, reinterpret_cast<const CryptoPP::byte*>(input.c_str()), input.size());
     CryptoPP::HexEncoder encoder;
     std::string hexOutput;
     encoder.Attach(new CryptoPP::StringSink(hexOutput));
     encoder.Put(digest, sizeof(digest));
     encoder.MessageEnd();
     return hexOutput;

}
