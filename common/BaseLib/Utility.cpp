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
#include <boost_unordered.hpp>

namespace Utility
{
    using enum fmt::color;
    using enum EOrder;
    boost::unordered_flat_set<EOrder> blacklist_orders =
    {
        NONE,
        ID_PING,
        ID_PONG,
        INFO_INVENTORY,
        USER_MOVE,
        HOST_NPC_MOVE,
        INFO_OTHER_USER_MOVE,
        USER_NICKNAME,
        ROOM_LIST
    };

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
    static const boost::unordered_flat_map<std::string, int> kTzOffsets = {
    {"UTC", 0}, {"GMT", 0},
    {"EST", -5}, {"CST", -6}, {"MST", -7}, {"PST", -8},
    {"EET", +2}, {"EEST", +3},
    {"CET", +1}, {"CEST", +2},
    {"GMT-12",-12},{"GMT-11",-11},{"GMT-10",-10},{"GMT-9",-9},{"GMT-8",-8},
    {"GMT-7",-7},{"GMT-6",-6},{"GMT-5",-5},{"GMT-4",-4},{"GMT-3",-3},
    {"GMT-2",-2},{"GMT-1",-1},{"GMT+0",0},{"GMT+1",+1},{"GMT+2",+2},
    {"GMT+3",+3},{"GMT+4",+4},{"GMT+5",+5},{"GMT+6",+6},{"GMT+7",+7},
    {"GMT+8",+8},{"GMT+9",+9},{"GMT+10",+10},{"GMT+11",+11},{"GMT+12",+12}
    };

    namespace Random
    {
        thread_local std::mt19937_64 g_eng{};
        thread_local bool g_seeded = false;
        void Init()
        {
            if (g_seeded) return;
            std::random_device rd{};
            std::array<std::uint32_t, 8> seed_data{
                rd(), rd(), rd(), rd(),
                rd(), rd(), rd(), rd()
            };
            std::seed_seq seq(seed_data.begin(), seed_data.end());
            g_eng.seed(seq);
            g_seeded = true;
        }
        void SetSeed(std::uint64_t seed) noexcept
        {
            g_eng.seed(static_cast<std::mt19937::result_type>(seed));
            g_seeded = true;
        }
        std::uint32_t CustomGen(std::uint32_t min, std::uint32_t max) noexcept
        {
            Init();
            std::uniform_int_distribution<std::uint32_t> dist(min, max);
            return dist(g_eng);
        }
        std::uint64_t CustomGen64(std::uint64_t min, std::uint64_t max) noexcept
        {
            Init();
            std::uniform_int_distribution<std::uint64_t> dist(min, max);
            return dist(g_eng);
        }
        std::uint32_t Gen() noexcept
        {
            return CustomGen(0, UINT32_MAX);
        }
        std::uint64_t Gen64() noexcept
        {
            return CustomGen64(0, UINT64_MAX);
        }
    }
    namespace SecureRandomBlake2b
    {
        int64_t Generator::GetNanoTime()
        {
            auto now = std::chrono::high_resolution_clock::now();
            return std::chrono::duration_cast<std::chrono::nanoseconds>(now.time_since_epoch()).count();
        }
        void Generator::UInt64ToLE(uint64_t n, uint8_t* bs)
        {
            for (auto i = 0; i < 8; ++i)
                bs[i] = static_cast<uint8_t>(n >> (i * 8));
        }
        void Generator::AddCounter(int64_t seedVal)
        {
            uint8_t bytes[8];
            UInt64ToLE(static_cast<uint64_t>(seedVal), bytes);
            crypto_blake2b_ctx ctx;
            crypto_blake2b_init(&ctx, DIGEST_SIZE);
            crypto_blake2b_update(&ctx, bytes, sizeof(bytes));
            crypto_blake2b_update(&ctx, _seed, DIGEST_SIZE);
            crypto_blake2b_final(&ctx, _seed);
        }
        int64_t Generator::NextCounterValue()
        {
            return ++_counter;
        }
        void Generator::AddSeedMaterial(const uint8_t* inSeed, size_t length)
        {
            crypto_blake2b_ctx ctx;
            crypto_blake2b_init(&ctx, DIGEST_SIZE);
            crypto_blake2b_update(&ctx, inSeed, length);
            crypto_blake2b_update(&ctx, _seed, DIGEST_SIZE);
            crypto_blake2b_final(&ctx, _seed);
        }
        void Generator::AddSeedMaterial(int64_t rSeed)
        {
            AddCounter(rSeed);
            crypto_blake2b_ctx ctx;
            crypto_blake2b_init(&ctx, DIGEST_SIZE);
            crypto_blake2b_update(&ctx, _seed, DIGEST_SIZE);
            crypto_blake2b_final(&ctx, _seed);
        }
        void Generator::CycleSeed()
        {
            crypto_blake2b_ctx ctx;
            crypto_blake2b_init(&ctx, DIGEST_SIZE);
            crypto_blake2b_update(&ctx, _seed, DIGEST_SIZE);
            AddCounter(_seedCounter++);
            crypto_blake2b_update(&ctx, _seed, DIGEST_SIZE);
            crypto_blake2b_final(&ctx, _seed);
        }
        void Generator::GenerateState()
        {
            AddCounter(_stateCounter++);
            crypto_blake2b_ctx ctx;
            crypto_blake2b_init(&ctx, DIGEST_SIZE);
            crypto_blake2b_update(&ctx, _state, DIGEST_SIZE);
            crypto_blake2b_update(&ctx, _seed, DIGEST_SIZE);
            crypto_blake2b_final(&ctx, _state);
            if ((_stateCounter % _rounds) == 0) CycleSeed();
        }
        void Generator::SetSeed(const uint8_t * seed, size_t length)
        {
            _seedCounter++;
            AddSeedMaterial(seed, length);
        }
        void Generator::SetSeed(int64_t seed)
        {
            _seedCounter++;
            AddSeedMaterial(seed);
        }
        void Generator::NextBytes(uint8_t * bytes, size_t length)
        {
            if (_seedCounter == 0) 
                throw std::runtime_error("_seedCounter == 0");

            size_t stateOff = 0;
            GenerateState();

            for (size_t i = 0; i < length; ++i) 
            {
                if (stateOff == DIGEST_SIZE)
                {
                    GenerateState();
                    stateOff = 0;
                }
                bytes[i] = _state[stateOff++];
            }
        }
        uint64_t Generator::NextUInt64()
        {
            uint8_t bytes[8];
            NextBytes(bytes, 8);

            uint64_t result = 0;
            for (auto i = 7; i >= 0; --i) 
                result = (result << 8) | bytes[i];
            return result;
        }
        uint32_t Generator::NextUInt32()
        {
            uint8_t bytes[4];
            NextBytes(bytes, 4);

            uint32_t result = 0;
            for (auto i = 3; i >= 0; --i)  
                result = (result << 8) | bytes[i];
            return result;
        }
        uint64_t Generator::GenerateAuthKey()
        {
            uint64_t key = 0;
            do 
            {
                key = NextUInt64();
            } while (key == 0);
            return key;
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

    [[nodiscard]]
    std::string readable_time(uint64_t ns)
    {
        using namespace std::chrono;
        const nanoseconds t{ns};
        constexpr int prec = 2;
        struct Unit 
        {
            nanoseconds          factor;
            std::string_view     name;
        };
        constexpr std::array<Unit, 6> table{ {
            { 1h,  "h"       },
            { 1min,"min"     },
            { 1s,  "s"     },
            { 1ms, "ms"},
            { 1us, "us"},
            { 1ns, "ns" }
            } };

        const auto it = std::ranges::find_if(table, [t](const Unit& u){ return t >= u.factor; });

        const double value = t / (1.0 * it->factor);
        return std::format("{:.{}f} {}", value, prec, it->name);
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
    uint32_t GetCurrentYear()
    {
        auto now = std::chrono::system_clock::now();
        std::time_t now_c = std::chrono::system_clock::to_time_t(now);
        std::tm now_tm;
    #ifdef _WIN64
        localtime_s(&now_tm, &now_c);
    #else
        localtime_r(&now_c, &now_tm);
    #endif
        return static_cast<uint32_t>(now_tm.tm_year + 1900);
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

        return std::format("{} Day(s) {} Hour(s) {} Minute(s)", days, hours, minutes);
    }
    uint64_t GetUtcTimeNowInSeconds()
    {
        auto now = std::chrono::system_clock::now();
        auto durationSinceEpoch = now.time_since_epoch();
        auto millis = duration_cast<std::chrono::seconds>(durationSinceEpoch).count();
        return static_cast<uint64_t>(millis);
    }
    int ParseOffsetHours(std::string_view tz) {
        if (auto it = kTzOffsets.find(std::string(tz)); it != kTzOffsets.end())
            return it->second;

		if (tz.starts_with("GMT") || tz.starts_with("UTC")) {
            if (tz.size() == 3) return 0;
            if (tz.size() >= 5 && (tz[3] == '+' || tz[3] == '-')) {
                return std::stoi(std::string(tz.substr(3)));
            }
        }
        throw std::invalid_argument("Unknown time zone key: " + std::string(tz));
    }

    std::string GetReadableTime(uint32_t time, std::string time_zone)
    {
        const int offset_h = ParseOffsetHours(time_zone);

        using namespace std::chrono;
        auto tp = system_clock::time_point{ seconds{time} } + hours{ offset_h };

        std::time_t tt = system_clock::to_time_t(tp);
        std::tm tm = *std::gmtime(&tt);

        const auto ms = std::chrono::duration_cast<std::chrono::milliseconds>(
            tp.time_since_epoch()) % std::chrono::milliseconds(1000);

        return fmt::format("{:02}-{:02}-{:04} {:02}:{:02}:{:02}.{:03}",
            tm.tm_mday,
            tm.tm_mon + 1,
            tm.tm_year + 1900,
            tm.tm_hour,
            tm.tm_min,
            tm.tm_sec,
            static_cast<int>(ms.count())
        );
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

    uint64_t GenerateAuthKey(const std::string& username, const std::string& password, const uint8_t* salt)
    {
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
#ifdef _WIN64
        SYSTEM_INFO info;
        GetSystemInfo(&info);
        return info.dwNumberOfProcessors;
#else
        const long n = sysconf(_SC_NPROCESSORS_ONLN);
        return n > 0 ? static_cast<uint32_t>(n) : 1u;
#endif
    }();
   
    void LogPackets(std::source_location source_location, BaseLib::PacketDir dir, NetEngine::CMessage& packetMessage, uint16_t m_sessionId)
    {
		auto order = magic_enum::enum_cast<EOrder>(NetEngine::u16_cast(packetMessage.GetOrder())).value_or(EOrder::NONE);
        if (blacklist_orders.contains(order)) return;
        std::string data_buffer;
        data_buffer.reserve(static_cast<size_t>(4 + 4 + packetMessage.GetDataSize() * 3));

        for (uint32_t i = 0; i < 4; i++)
            std::format_to(std::back_inserter(data_buffer), "{:02X} ", (unsigned char)(packetMessage.GetHeader().data >> (i * 8)));

        for (uint32_t i = 0; i < 4; i++)
            std::format_to(std::back_inserter(data_buffer), "{:02X} ", (unsigned char)(packetMessage.GetCommand().data >> (i * 8)));

        for (uint32_t i = 0; i < packetMessage.GetDataSize(); i++)
        {
            std::format_to(std::back_inserter(data_buffer), "{:02X}", (unsigned char)packetMessage.GetData()[i]);
            if (i != packetMessage.GetDataSize() - 1)
                data_buffer += ' ';
        }
        BaseLib::EventLog->Debug(source_location, dir, order, dark_cyan, "size=({:d}) msgSid=({}) sid=({}) mission=({}) extra=({}) option=({})\n{:s}", packetMessage.GetDataSize() + 8, packetMessage.GetSession(), m_sessionId, packetMessage.GetMission(), packetMessage.GetExtra(), packetMessage.GetOption(), data_buffer);
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
            BaseLib::EventLog->Debug(source_location, BaseLib::PacketDir::DEBUG, EOrder::NONE, dark_cyan, "Error loading file ({}): {}", filepath.c_str(), e.what());
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
            .algorithm = CRYPTO_ARGON2_ID,
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
