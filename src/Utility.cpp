#include "Utility.h"

namespace Utility
{
    std::map<std::string, std::string> time_zones = {
       {"UTC", "UTC"},
       {"EST", "UTC-5"},
       {"CST", "UTC-6"},
       {"MST", "UTC-7"},
       {"PST", "UTC-8"},
       {"GMT+1", "UTC-1"},
       {"GMT+2", "UTC-2"},
       {"GMT+3", "UTC-3"},
       {"GMT+4", "UTC-4"},
       {"GMT+5", "UTC-5"},
       {"GMT+6", "UTC-6"},
       {"GMT+7", "UTC-7"},
       {"GMT+8", "UTC-8"},
       {"GMT+9", "UTC-9"},
       {"GMT+10", "UTC-10"},
       {"GMT+11", "UTC-11"},
       {"GMT+12", "UTC-12"}
    };
    namespace Random
    {
        namespace Random
        {
            std::random_device rd;
            std::mt19937 rng(rd());
            std::mt19937_64 rng64(rd());
            std::uniform_int_distribution<std::uint32_t> dist;
            std::uniform_int_distribution<std::uint64_t> dist64;
            std::uint32_t Gen()
            {
                return dist(rng);
            }
            std::uint64_t Gen64()
            {
                return dist64(rng64);
            }
        }
    }

    std::uint32_t GetUtcTimeNow()
    {
        auto now = std::chrono::system_clock::now();
        auto now_c = std::chrono::system_clock::to_time_t(now);
        return static_cast<std::uint32_t>(now_c);
    }
    std::string GetReadableTime(std::uint32_t time, std::string time_zone)
    {
        auto& time_offset = time_zones[time_zone];
        auto offset = std::stoi(time_offset.substr(4));
        auto time_point = std::chrono::system_clock::from_time_t(time);

        time_point += std::chrono::hours(offset);
        std::time_t time_t_time = std::chrono::system_clock::to_time_t(time_point);
        std::tm tm_time;
        localtime_s(&tm_time, &time_t_time);
        std::stringstream ss;
        ss << std::put_time(std::localtime(&time_t_time), "%c %Z");
        return ss.str();
    }
    std::string GetBytesArray(std::uint8_t* data, std::uint16_t size)
    {
        std::stringstream ss;
        ss << std::hex;
        for (std::size_t i = 0; i < size; ++i) {
            ss << std::setw(2) << std::setfill('0') << (int)data[i] << ' ';
        }
        return ss.str();
    }
    std::string ToLowercase(const std::string& str)
    {
        std::string result = str;
        std::transform(result.begin(), result.end(), result.begin(),
            [](unsigned char c) { return std::tolower(c); });
        return result;
    }
    void ToLowercase(std::string& str)
    {
        std::transform(str.begin(), str.end(), str.begin(),
            [](unsigned char c) { return std::tolower(c); });
    }
    std::uint64_t GenerateAuthKey(const std::string& username, const std::string& password)
    {
        std::uint64_t auth_key = 0;
        unsigned char hash[EVP_MAX_MD_SIZE];
        unsigned int hash_len = 0;
        std::string data = username + password;
        const EVP_MD* md = EVP_sha3_256();
        EVP_MD_CTX* mdctx = EVP_MD_CTX_new();
        EVP_DigestInit_ex(mdctx, md, NULL);
        EVP_DigestUpdate(mdctx, data.c_str(), data.length());
        EVP_DigestFinal_ex(mdctx, hash, &hash_len);
        EVP_MD_CTX_free(mdctx);
        std::memcpy(&auth_key, hash, sizeof(auth_key));

        return auth_key;
    }
    const int kIterations = 30000;
    const int kSaltLength = 24;
    const int kHashLength = 24;
    bool IsPasswordValid(const std::string& password, const std::string& hash, const std::string& salt)
    {
        if (password.empty() || hash.empty() || salt.empty())
            return false;

        std::vector<unsigned char> actualPasswordHash = DecodeBase64(hash);
        std::vector<unsigned char> actualPasswordSalt = DecodeBase64(salt);
        std::vector<unsigned char> passwordGuess(kHashLength);

        PKCS5_PBKDF2_HMAC(password.c_str(), password.length(),
            reinterpret_cast<const unsigned char*>(actualPasswordSalt.data()), actualPasswordSalt.size(),
            kIterations, EVP_sha256(),
            kHashLength, passwordGuess.data());

        return actualPasswordHash == passwordGuess;
    }
    std::pair<std::string, std::string> Hash(const std::string& password)
    {
        std::vector<unsigned char> salt(kSaltLength);
        RAND_bytes(salt.data(), kSaltLength);

        std::vector<unsigned char> hash(kHashLength);
        PKCS5_PBKDF2_HMAC(password.c_str(), password.length(),
            salt.data(), kSaltLength,
            kIterations, EVP_sha256(),
            kHashLength, hash.data());

        return { EncodeBase64(hash), EncodeBase64(salt) };
    }

    std::vector<unsigned char> DecodeBase64(const std::string& str)
    {
        BIO* b64 = BIO_new(BIO_f_base64());
        BIO_set_flags(b64, BIO_FLAGS_BASE64_NO_NL);
        BIO* bio = BIO_new_mem_buf(str.data(), str.length());
        bio = BIO_push(b64, bio);
        std::vector<unsigned char> output(str.length());
        int len = BIO_read(bio, output.data(), output.size());
        output.resize(len);
        BIO_free_all(bio);
        return output;
    }

    std::string EncodeBase64(const std::vector<unsigned char>& data)
    {
        BIO* b64 = BIO_new(BIO_f_base64());
        BIO_set_flags(b64, BIO_FLAGS_BASE64_NO_NL);
        BIO* bio = BIO_new(BIO_s_mem());
        bio = BIO_push(b64, bio);
        BIO_write(bio, data.data(), data.size());
        BIO_flush(bio);
        char* ptr;
        long len = BIO_get_mem_data(bio, &ptr);
        std::string output(ptr, len);
        BIO_free_all(bio);
        return output;
    }

}