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
    Random::Random() : rng(std::random_device{}()) {}
    std::uint32_t Random::Gen() { return this->dist(rng); }

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
        time += offset * 3600;
        auto timeinfo = std::gmtime(reinterpret_cast<const time_t*>(&time));
        char buffer[80];
        std::strftime(buffer, 80, "%c %Z", timeinfo);
        return buffer;
    }
    void PrintBytes(std::ostream& out, const char* title, const unsigned char* data, size_t dataLen, bool format)
    {
        out << title << std::endl;
        out << std::setfill('0');

        for (size_t i = 0; i < dataLen; ++i) {
            out << std::hex << std::setw(2) << (int)data[i];
            if (format) {
                out << (((i + 1) % 16 == 0) ? "\n" : " ");
            }
        }

        out << std::endl;
    }

}