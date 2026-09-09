#pragma once
#include <cstdint>
#include <string>
std::string sha1(const std::string& input);
std::string urlEncodeHex(const std::string& input);
std::string hexDecode(const std::string& value);
std::string bytesToIpAddress(const std::string& bytes);
long long bytesToPort(const std::string& bytes);