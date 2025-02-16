#pragma once
#include <iostream>
#include <filesystem>
#include <string>
#include <random>
#include <chrono>
#include <map>
#include <sstream>
#ifdef _WIN64
#pragma comment(lib, "libcrypto.lib")
#endif
#include <openssl/evp.h>
#include <openssl/rand.h>
#include <openssl/sha.h>
#include <fmt/format.h>
#include <fmt/color.h>
#include <NetEngine/CMessage.h>
#include <source_location>
namespace Utility
{
    constexpr std::string_view allowedChars = "abcdefghijklmnopqrstuvwxyz0123456789~!@#$%^&*()-_=+[{]}<.>/?";
    constexpr std::string_view forbiddenSubstrings[] = { "[gm]", "[mod]", "{gm}", "(gm)", "{mod}", "(mod)", "admin", "administrator", "gamemaster", "moderator", "retard", "jew", "nigger", "nigga", "faggot", "tranny" };
    bool ContainsForbiddenSubstring(std::string_view str);
    bool IsValidNickname(char nickname[16]);
    bool IsValidNickname(const std::string_view nicknameView);
    
    namespace Random
    {
        std::uint32_t Gen();
        std::uint64_t Gen64();
        std::uint32_t CustomGen(const std::uint32_t min, const std::uint32_t max);
        std::uint64_t CustomGen64(const std::uint64_t min, const std::uint64_t max);
    }
    
    std::string round_float(float var);
    std::string readable_size(std::uint64_t bytes);
    std::string readable_time(std::uint64_t ns);
    std::uint32_t GetUnixEpoch();
    std::tm ConvertUtcTimestampToDate(std::uint64_t timestamp);
    std::uint32_t GetUtcTimeNow();
    std::uint32_t GetCurrentMonth();
    std::uint32_t GetCurrentDay();
    std::uint32_t GetUtcTimeNowPlusSeconds(const std::uint32_t& seconds);
    std::uint64_t GetUtcTimeNow64();
    std::uint64_t GetUtcTimeNowInMilliseconds();
    std::string FormatMilliseconds(std::uint64_t milliseconds);
    std::uint64_t GetUtcTimeNowInSeconds();
    std::string GetReadableTime(std::uint32_t time, std::string time_zone);
    std::uint64_t DateTimeToUInt64(const std::string& formatted_datetime);
    std::string UInt64ToDateTimeString(std::uint64_t unix_timestamp);
    std::string GetBytesArray(std::uint8_t* data, std::uint16_t size);
    std::uint64_t GenerateAuthKey(const std::string& username, const std::string& password);
    std::string ToLowercase(const std::string& str);
    void ToLowercase(std::string& str);
    bool IsPasswordValid(const std::string& password, const std::string& hash, const std::string& salt);
    std::string ReadMVString(std::string_view in);
    std::string ReadMicrovoltsString(const char* data, std::uint32_t size);
    std::pair<std::string, std::string> Hash(const std::string& password);
    std::vector<unsigned char> DecodeBase64(const std::string& str);
    std::string EncodeBase64(const std::vector<unsigned char>& data);
    std::vector<std::string> SplitStrings(std::string_view str, char delimiter);
    bool IsDigitsOnly(const std::string& input);
    std::uint32_t ExtractNumber(const std::string& input);
    template <typename T>
    std::vector<std::uint8_t> ToVector(const T& data)
    {
        auto bytes = std::bit_cast<std::array<std::uint8_t, sizeof(T)>>(data);
        return std::vector<std::uint8_t>(bytes.begin(), bytes.end());
    }
    template <typename T>
    T FromVector(const std::vector<std::uint8_t>& bytes) 
    {
        T object;
        std::memcpy(&object, bytes.data(), sizeof(T));
        return object;
    }
    void LogPackets(std::source_location source_location, NetEngine::CMessage& packetMessage, std::uint16_t m_sessionId);
    std::vector<std::uint8_t> load_file(const std::string& filepath);
    double GetCpuUsage(void* m_process_handle);
    std::int64_t GetMemoryUsage(void* m_process_handle);
}


//#endif