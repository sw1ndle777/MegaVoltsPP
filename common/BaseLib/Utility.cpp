#include "CLog.h"
#include "Utility.h"
#ifdef _WIN64
#define WIN32_LEAN_AND_MEAN
#include <Windows.h>
#include <processthreadsapi.h>
#include <Psapi.h>
#else
#include <unistd.h>
#include <sys/resource.h>
#include <sys/time.h>
#endif

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
        std::random_device rd;
        std::mt19937 rng(rd());
        std::mt19937_64 rng64(rd());
        std::uniform_int_distribution<uint32_t> dist;
        std::uniform_int_distribution<uint64_t> dist64;
        uint32_t Gen()
        {
            return dist(rng);
        }
        uint64_t Gen64()
        {
            return dist64(rng64);
        }
        uint32_t CustomGen(const uint32_t min, const uint32_t max)
        {
            std::uniform_int_distribution<uint32_t> custom_dist(min, max);
            return custom_dist(rng);
        }
        uint64_t CustomGen64(const uint64_t min, const uint64_t max)
        {
            std::uniform_int_distribution<uint64_t> custom_dist64(min, max);
            return custom_dist64(rng64);
        }
    }
    std::string round_float(float var)
    {
        std::ostringstream out;
        out << std::fixed << std::setprecision(2) << var;
        return out.str();
    }

    std::string readable_size(uint64_t bytes)
    {
        const uint64_t gb = 1073741824;
        const uint64_t mb = 1048576;
        const uint64_t kb = 1024;
        if (bytes >= gb) return round_float(static_cast<float>(bytes) / gb) + " GB ";
        if (bytes >= mb) return round_float(static_cast<float>(bytes) / mb) + " MB ";
        if (bytes >= kb) return round_float(static_cast<float>(bytes) / kb) + " KB ";
        return std::to_string(bytes) + " B ";
    }

    std::string readable_time(uint64_t ns)
    {
        const uint64_t nanoseconds = 1;
        const uint64_t microseconds = 1000 * nanoseconds;
        const uint64_t milliseconds = 1000 * microseconds;
        const uint64_t seconds = 1000 * milliseconds;
        const uint64_t minutes = 60 * seconds;
        const uint64_t hours = 60 * minutes;

        auto format_float = [](float number, int precision = 2) -> std::string
        {
            std::ostringstream out;
            out << std::fixed << std::setprecision(precision) << number;
            return out.str();
        };

        if (ns >= hours) return format_float(static_cast<float>(ns) / hours) + " hours ";
        if (ns >= minutes) return format_float(static_cast<float>(ns) / minutes) + " minutes ";
        if (ns >= seconds) return format_float(static_cast<float>(ns) / seconds) + " seconds ";
        if (ns >= milliseconds) return format_float(static_cast<float>(ns) / milliseconds) + " milliseconds ";
        if (ns >= microseconds) return format_float(static_cast<float>(ns) / microseconds) + " microseconds ";
        return std::to_string(ns) + " nanoseconds ";
    }
    bool ContainsForbiddenSubstring(std::string_view str)
    {
        for (const auto& forbidden : forbiddenSubstrings)
        {
            if (str.find(forbidden) != std::string_view::npos)
                return true;
        }
        return false;
    }
    bool IsValidNickname(char nickname[16])
    {
        std::string nicknameStr{ nickname };
        ToLowercase(nicknameStr);
        std::string_view nicknameView{ nicknameStr };

        for (char c : nicknameView)
            if (allowedChars.find(c) == std::string_view::npos)
                return false;

        if (ContainsForbiddenSubstring(nicknameView)) return false;

        return true;
    }

    bool IsValidNickname(const std::string_view nicknameView)
    {
        std::string lowercaseNickname = ToLowercase(std::string(nicknameView));

        for (char c : lowercaseNickname)
            if (allowedChars.find(c) == std::string_view::npos)
                return false;

        if (ContainsForbiddenSubstring(lowercaseNickname)) return false;

        return true;
    }

    uint32_t GetUnixEpoch()
    {
        auto now = std::chrono::system_clock::from_time_t(0);
        auto now_c = std::chrono::system_clock::to_time_t(now);
        return static_cast<uint32_t>(now_c);
    }
    uint32_t GetUtcTimeNow()
    {
        auto now = std::chrono::system_clock::now();
        auto now_c = std::chrono::system_clock::to_time_t(now);
        return static_cast<uint32_t>(now_c);
    }
    uint32_t GetCurrentMonth()
    {
        auto now = std::chrono::system_clock::now();
        std::time_t now_c = std::chrono::system_clock::to_time_t(now);
        std::tm now_tm;
    #ifdef _WIN64
        localtime_s(&now_tm, &now_c);
    #else
        localtime_r(&now_c, &now_tm);
    #endif
        return static_cast<uint32_t>(now_tm.tm_mon + 1);
    }
    uint32_t GetCurrentDay()
    {
        auto now = std::chrono::system_clock::now();
        std::time_t now_c = std::chrono::system_clock::to_time_t(now);
        std::tm now_tm;
    #ifdef _WIN64
        localtime_s(&now_tm, &now_c);
    #else
        localtime_r(&now_c, &now_tm);
    #endif
        return static_cast<uint32_t>(now_tm.tm_mday);
    }
    uint32_t GetUtcTimeNowPlusSeconds(const uint32_t& seconds)
    {
        auto now = std::chrono::system_clock::now();
        auto future_time = now + std::chrono::seconds(seconds);
        auto future_time_c = std::chrono::system_clock::to_time_t(future_time);
        return static_cast<uint32_t>(future_time_c);
    }
    std::tm ConvertUtcTimestampToDate(uint64_t timestamp)
    {
        std::time_t time = static_cast<std::time_t>(timestamp);
        std::tm date;
    #ifdef _WIN64
        // Windows: Use gmtime_s
        gmtime_s(&date, &time);
    #else
        // Linux/Unix: Use gmtime_r
        gmtime_r(&time, &date);
    #endif
        return date;
    }
    uint64_t GetUtcTimeNow64()
    {
        auto now = std::chrono::system_clock::now();
        auto now_c = std::chrono::system_clock::to_time_t(now);
        return static_cast<uint64_t>(now_c);
    }
    uint64_t GetLast6AMUtc()
    {
        // Get current UTC time
        auto now = std::chrono::system_clock::now();
        std::time_t now_c = std::chrono::system_clock::to_time_t(now);

        // Convert to UTC time structure
        std::tm utc_tm = *std::gmtime(&now_c);

        // Set time to 6 AM of today
        utc_tm.tm_hour = 6;
        utc_tm.tm_min = 0;
        utc_tm.tm_sec = 0;

        // Convert back to time_t
        std::time_t six_am_today = std::mktime(&utc_tm);

        // If current time is before 6 AM, subtract one day
        if (now_c < six_am_today)
        {
            six_am_today -= 86400; // Subtract 24 hours in seconds
        }

        // Return as uint64_t
        return static_cast<uint64_t>(six_am_today);
    }
    uint64_t GetUtcTimeNowInMilliseconds()
    {
        auto now = std::chrono::system_clock::now();
        auto durationSinceEpoch = now.time_since_epoch();
        auto millis = duration_cast<std::chrono::milliseconds>(durationSinceEpoch).count();
        return static_cast<uint64_t>(millis);
    }
    std::string FormatMilliseconds(uint64_t milliseconds)
    {
        constexpr uint64_t ms_in_a_second = 1000;
        constexpr uint64_t ms_in_a_minute = ms_in_a_second * 60;
        constexpr uint64_t ms_in_an_hour = ms_in_a_minute * 60;
        constexpr uint64_t ms_in_a_day = ms_in_an_hour * 24;

        uint64_t days = milliseconds / ms_in_a_day;
        milliseconds %= ms_in_a_day;

        uint64_t hours = milliseconds / ms_in_an_hour;
        milliseconds %= ms_in_an_hour;

        uint64_t minutes = milliseconds / ms_in_a_minute;

        return fmt::format("{} Day(s) {} Hour(s) {} Minute(s)", days, hours, minutes);
    }
    uint64_t GetUtcTimeNowInSeconds()
    {
        auto now = std::chrono::system_clock::now();
        auto durationSinceEpoch = now.time_since_epoch();
        auto millis = duration_cast<std::chrono::seconds>(durationSinceEpoch).count();
        return static_cast<uint64_t>(millis);
    }
    std::string GetReadableTime(uint32_t time, std::string time_zone)
    {
        auto& time_offset = time_zones[time_zone];
        auto offset = std::stoi(time_offset.substr(4));
        auto time_point = std::chrono::system_clock::from_time_t(time);

        time_point += std::chrono::hours(offset + 1);
        std::time_t truncated_time = std::chrono::system_clock::to_time_t(time_point);
        std::tm tm_time = *std::gmtime(&truncated_time);

        return fmt::format("{:04d}-{:02d}-{:02d} {:02d}:{:02d}:{:02d}",
                           tm_time.tm_year + 1900, tm_time.tm_mon + 1, tm_time.tm_mday,
                           tm_time.tm_hour, tm_time.tm_min, tm_time.tm_sec);
    }
    uint64_t DateTimeToUInt64(const std::string& formatted_datetime)
    {
        std::tm tm_time{};
        std::istringstream ss(formatted_datetime);
        char delimiter;
        ss >> tm_time.tm_year >> delimiter >> tm_time.tm_mon >> delimiter >> tm_time.tm_mday
            >> tm_time.tm_hour >> delimiter >> tm_time.tm_min >> delimiter >> tm_time.tm_sec;

        tm_time.tm_year -= 1900;
        tm_time.tm_mon -= 1;
        std::time_t time_t_time = std::mktime(&tm_time);
        return static_cast<uint64_t>(time_t_time);
    }
    std::string UInt64ToDateTimeString(uint64_t unix_timestamp)
    {
        // If unix_timestamp is 0, use the current time
        if (unix_timestamp == 0)
        {
            unix_timestamp = std::chrono::system_clock::to_time_t(std::chrono::system_clock::now());
        }

        std::chrono::seconds duration(unix_timestamp);
        std::chrono::time_point<std::chrono::system_clock> time_point(duration);
        std::time_t time_t_timestamp = std::chrono::system_clock::to_time_t(time_point);
        std::tm* tm_time = std::localtime(&time_t_timestamp);

        if (tm_time == nullptr)
        {
            return "Invalid timestamp";
        }

        std::ostringstream ss;
        ss << std::put_time(tm_time, "%Y-%m-%d %H:%M:%S");
        return ss.str();
    }
    std::string GetBytesArray(uint8_t* data, uint16_t size)
    {
        std::stringstream ss;
        ss << std::hex;
        for (size_t i = 0; i < size; ++i)
        {
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
    uint64_t GenerateAuthKey(const std::string& username, const std::string& password, const uint8_t* salt)
    {
        /*
        uint64_t auth_key = 0;
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
        */
        crypto_argon2_config config = {
            .algorithm  = CRYPTO_ARGON2_ID,  // ID = hybrid variant
            .nb_blocks  = 1000,              // Less intensive for fast auth key
            .nb_passes  = 2,
            .nb_lanes   = 1
        };

        std::string combined = username + password;
        crypto_argon2_inputs inputs = {
            .pass       = reinterpret_cast<const uint8_t*>(combined.data()),
            .salt       = salt,
            .pass_size  = static_cast<uint32_t>(combined.size()),
            .salt_size  = 16
        };

        crypto_argon2_extras extras = {0};

        void* work_area = malloc((size_t)config.nb_blocks * 1024);
        if (!work_area)
            return 0;

        uint8_t hash[32];
        crypto_argon2(hash, 32, work_area, config, inputs, extras);

        crypto_wipe(work_area, config.nb_blocks * 1024);
        free(work_area);

        // Take the first 8 bytes of hash and cast to uint64_t
        uint64_t auth_key = 0;
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

        /*
        std::vector<unsigned char> actualPasswordHash = DecodeBase64(hash);
        std::vector<unsigned char> actualPasswordSalt = DecodeBase64(salt);
        std::vector<unsigned char> passwordGuess(kHashLength);

        PKCS5_PBKDF2_HMAC(password.c_str(), password.length(),
            reinterpret_cast<const unsigned char*>(actualPasswordSalt.data()), actualPasswordSalt.size(),
            kIterations, EVP_sha256(),
            kHashLength, passwordGuess.data());
        */

        //return actualPasswordHash == passwordGuess;

        return password._Equal(hash.c_str());
    }
    std::string ReadMicrovoltsString(const char* data, uint32_t size)
    {
        if (data == nullptr || size <= 0)
            return "";
        std::vector<char> bytes(data, data + size);
        auto terminatorPosition = std::find(bytes.begin(), bytes.end(), '\0');
        size_t stringLength = (terminatorPosition != bytes.end()) ? std::distance(bytes.begin(), terminatorPosition) : size;
        return std::string(bytes.begin(), bytes.begin() + stringLength);
    }
    std::string ReadMVString(std::string_view in)
    {
        auto pos = in.find('\0');
        if (pos == in.npos) { return ""; }
        return { in.data(), (size_t)pos };
    }
    /*
    std::pair<std::string, std::string> Hash(const std::string& password)
    {

        std::vector<unsigned char> salt(kSaltLength);
        RAND_bytes(salt.data(), kSaltLength);

        std::vector<unsigned char> hash(kHashLength);
        PKCS5_PBKDF2_HMAC(password.c_str(), static_cast<int32_t>(password.length()),
                          salt.data(), kSaltLength,
                          kIterations, EVP_sha256(),
                          kHashLength, hash.data());

        //return { EncodeBase64(hash), EncodeBase64(salt) };
        return { password,password };
    }

    std::vector<unsigned char> DecodeBase64(const std::string& str)
    {
        BIO* b64 = BIO_new(BIO_f_base64());
        BIO_set_flags(b64, BIO_FLAGS_BASE64_NO_NL);
        BIO* bio = BIO_new_mem_buf(str.data(), static_cast<int32_t>(str.length()));
        bio = BIO_push(b64, bio);
        std::vector<unsigned char> output(str.length());
        int len = BIO_read(bio, output.data(), static_cast<int32_t>(output.size()));
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
        BIO_write(bio, data.data(), static_cast<int32_t>(data.size()));
        BIO_flush(bio);
        char* ptr;
        long len = BIO_get_mem_data(bio, &ptr);
        std::string output(ptr, len);
        BIO_free_all(bio);
        return output;
    }
    */

    std::vector<std::string> SplitStrings(std::string_view str, char delimiter)
    {
        std::vector<std::string> result;
        size_t start = 0;
        size_t end = str.find(delimiter);

        while (end != std::string_view::npos)
        {
            if (end != start)  result.emplace_back(str.substr(start, end - start));
            start = end + 1;
            end = str.find(delimiter, start);
        }

        if (start < str.size()) result.emplace_back(str.substr(start));

        return result;
    }

    bool IsDigitsOnly(const std::string& input)
    {
        for (char ch : input)
            if (!std::isdigit(ch))
                return false;

        return true;
    }

    uint32_t ExtractNumber(const std::string& input)
    {
        return static_cast<uint32_t>(std::stoul(input));
    }
    std::int64_t cpu_last_time = 0;
    std::int64_t cpu_last_system_time = 0;

    uint32_t num_processors = []
    {
        SYSTEM_INFO info;
        GetSystemInfo(&info);
        return info.dwNumberOfProcessors;
    }();
    void LogPackets(std::source_location source_location, std::shared_ptr<NetEngine::CMessage>& packetMessage, uint16_t m_sessionId)
    {
        if (packetMessage->GetOrder() != 71 || packetMessage->GetOrder() != 72)
        {
            std::string data_buffer;
            data_buffer.reserve(static_cast<size_t>(4 + 4 + packetMessage->GetDataSize() * 3));

            for (uint32_t i = 0; i < 4; i++)
                fmt::format_to(std::back_inserter(data_buffer), "{:02X} ", (unsigned char)(packetMessage->GetHeader().data >> (i * 8)));

            for (uint32_t i = 0; i < 4; i++)
                fmt::format_to(std::back_inserter(data_buffer), "{:02X} ", (unsigned char)(packetMessage->GetCommand().data >> (i * 8)));

            for (uint32_t i = 0; i < packetMessage->GetDataSize(); i++)
            {
                fmt::format_to(std::back_inserter(data_buffer), "{:02X}", (unsigned char)packetMessage->GetData()[i]);
                if (i != packetMessage->GetDataSize() - 1)
                    data_buffer += ' ';
            }
            /*
            if (packetMessage.GetOrder() == 142 || packetMessage.GetOrder() == 71) {
                return;
            }
            */
            BaseLib::EventLog->Debug(source_location, fmt::color::dark_cyan, "({:d} bytes) MsgSessionId: {}, CSessionId: {}, Order: ({}), Mission: ({}), Extra: ({}), Option: ({})\n{:s}", packetMessage->GetDataSize() + 8, packetMessage->GetSession(), m_sessionId, packetMessage->GetOrder(), packetMessage->GetMission(), packetMessage->GetExtra(), packetMessage->GetOption(), data_buffer);
        }
    }
    std::vector<uint8_t> load_file(std::source_location source_location, const std::string& filepath)
    {
        try
        {
            std::ifstream ifs(filepath, std::ios::binary | std::ios::ate);

            auto end = ifs.tellg();
            ifs.seekg(0, std::ios::beg);

            auto size = static_cast<size_t>(end - ifs.tellg());

            if (size == 0)
                return {};

            std::vector<uint8_t> buffer(size);

            ifs.read(reinterpret_cast<char*>(buffer.data()), buffer.size());
            return buffer;
        }
        catch (const std::exception& e)
        {
            BaseLib::EventLog->Debug(source_location, fmt::color::dark_cyan, "Error loading file ({}): {}", filepath.c_str(), e.what());
            return {};
        }
    }
    double GetCpuUsage(void* m_process_handle)
    {
    #ifdef _WIN64
        auto fileTimeToUtc = [](const FILETIME* ftime) -> std::int64_t
        {
            LARGE_INTEGER li;
            li.LowPart = ftime->dwLowDateTime;
            li.HighPart = ftime->dwHighDateTime;
            return li.QuadPart;
        };

        FILETIME now, creation_time, exit_time, kernel_time, user_time;
        int64_t system_time, time, system_time_delta, time_delta;

        double cpu = -1;
        if (!m_process_handle) return -1;

        GetSystemTimeAsFileTime(&now);
        if (!GetProcessTimes(m_process_handle, &creation_time, &exit_time, &kernel_time, &user_time)) return -1;

        system_time = (fileTimeToUtc(&kernel_time) + fileTimeToUtc(&user_time)) / num_processors;
        time = fileTimeToUtc(&now);

        if ((cpu_last_system_time == 0) || (cpu_last_time == 0))
        {
            cpu_last_system_time = system_time;
            cpu_last_time = time;
            return -1;
        }

        system_time_delta = system_time - cpu_last_system_time;
        time_delta = time - cpu_last_time;

        cpu = (double)system_time_delta * 100 / (double)time_delta;
        cpu_last_system_time = system_time;
        cpu_last_time = time;
        return cpu;

    #else
        static long lastTotalUser, lastTotalUserLow, lastTotalSys, lastTotalIdle;
        long totalUser, totalUserLow, totalSys, totalIdle;
        double percent = -1.0;

        FILE* file = fopen("/proc/stat", "r");
        if (!file) return -1.0;

        fscanf(file, "cpu %ld %ld %ld %ld", &totalUser, &totalUserLow, &totalSys, &totalIdle);
        fclose(file);

        if (lastTotalUser || lastTotalSys || lastTotalIdle)
        {
            long totalDiff = (totalUser - lastTotalUser) + (totalUserLow - lastTotalUserLow) +
                (totalSys - lastTotalSys);
            long totalTime = totalDiff + (totalIdle - lastTotalIdle);
            percent = (totalDiff * 100.0) / totalTime;
        }

        lastTotalUser = totalUser;
        lastTotalUserLow = totalUserLow;
        lastTotalSys = totalSys;
        lastTotalIdle = totalIdle;
        return percent;
    #endif
    }

    std::int64_t GetMemoryUsage(void* m_process_handle)
    {
    #ifdef _WIN64
        int64_t memory_usage = 0;
        PROCESS_MEMORY_COUNTERS_EX pmc;

        if (GetProcessMemoryInfo(m_process_handle, (PROCESS_MEMORY_COUNTERS*)&pmc, sizeof(pmc)))
            memory_usage = pmc.PrivateUsage;
        else
            memory_usage = -1;

        return memory_usage / (1024 * 1024);

    #else
        struct rusage usage;
        if (getrusage(RUSAGE_SELF, &usage) == 0)
            return usage.ru_maxrss / 1024; // Convert from KB to MB
        return -1;
    #endif
    }

    bool HashPassword(const std::string& password, const uint8_t* salt, uint8_t* out_hash)
    {
        crypto_argon2_config config = {
            .algorithm = CRYPTO_ARGON2_I,
            .nb_blocks = 100000,
            .nb_passes = 3,
            .nb_lanes = 1
        };

        crypto_argon2_inputs inputs = {
            .pass = reinterpret_cast<const uint8_t*>(password.data()),
            .salt = salt,
            .pass_size = static_cast<uint32_t>(password.size()),
            .salt_size = 16
        };

        crypto_argon2_extras extras = { 0 };

        void* work_area = malloc(static_cast<size_t>(config.nb_blocks) * 1024);
        if (!work_area) return false;

        crypto_argon2(out_hash, 32, work_area, config, inputs, extras);
        crypto_wipe(work_area, config.nb_blocks * 1024);
        free(work_area);
        return true;
    }
}