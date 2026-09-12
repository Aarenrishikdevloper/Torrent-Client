# include "crypto++/sha.h"
#include "../includes/utils.hpp"
#include "crypto++/hex.h"
#include <string>

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

std::string urlEncodeHex(const std::string &input) {
    std::string result;
    const char* HexDigits = "0123456789ABCDEF";
    for (char ch: input) {
        if (isalnum(ch) || ch == '-' || ch == '_' || ch == '.' || ch == '~') {
            result += ch;
        }else {
            result += '%';
            result += HexDigits[(ch >> 4) & 0x0F];
            result += HexDigits[ch & 0x0F];
        }
    }
     return result;
}

std::string hexDecode(const std::string &value) {
    int hashLenght = value.length();
     std::string result;
     for (int i =0; i < hashLenght; i+= 2) {
         std::string byte = value.substr(i, 2);
         char c = static_cast<char>(static_cast<int>(strtol(byte.c_str(), nullptr, 16)));
         result.push_back(c);
     }
     return result;

}

std::string bytesToIpAddress(const std::string &bytes) {
    if (bytes.length() < 4) {
        return  "";
    }
     std::string ipAddress;
     for (int i =0; i < 4; ++i) {
         unsigned int byteValue = static_cast<unsigned char>(bytes[i]);
         ipAddress += std::to_string(byteValue);
         if (i < 3) {
             ipAddress += ".";
         }
     }
     return ipAddress;
}
long long bytesToPort(const std::string &bytes) {
     if (bytes.length() < 2) {
         return  0;

     }
     long long port  =0;
     port |= static_cast<unsigned char>(bytes[0]) <<8;
     port |= static_cast<unsigned char>(bytes[1]);
     return port;
 }
int getIntFromStr(const std::string& input) {
     if (input.length() < 4) {
         throw std::invalid_argument("Invalid input:: String does no contain 4 bytes");
     }
     const unsigned int result = (static_cast<unsigned char>(input[0])<< 24) | (static_cast<unsigned char>(input[1]) << 16) | (static_cast<unsigned char>(input[2]) << 8) | (static_cast<unsigned char>(input[3]));
     return  static_cast<int>(result);
 }