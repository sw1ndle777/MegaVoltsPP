#pragma once
#ifndef UTILITY_H
#define UTILITY_H

#include <iostream>
#include <filesystem>
#include <string>
#include <random>
#include <chrono>
#include <map>
#include <sstream>
#pragma comment(lib, "openssl/lib/libcrypto.lib")
#include <openssl/evp.h>
#include <openssl/rand.h>
#include <openssl/sha.h>
namespace Utility
{
    namespace Random
    {
        std::uint32_t Gen();
        std::uint64_t Gen64();
    }
    std::uint32_t GetUtcTimeNow();
    std::string GetReadableTime(std::uint32_t time, std::string time_zone);
    std::string GetBytesArray(std::uint8_t* data, std::uint16_t size);
    std::uint64_t GenerateAuthKey(const std::string& username, const std::string& password);
    std::string ToLowercase(const std::string& str);
    void ToLowercase(std::string& str);
    bool IsPasswordValid(const std::string& password, const std::string& hash, const std::string& salt);
    std::pair<std::string, std::string> Hash(const std::string& password);
    std::vector<unsigned char> DecodeBase64(const std::string& str);
    std::string EncodeBase64(const std::vector<unsigned char>& data);
}


#endif