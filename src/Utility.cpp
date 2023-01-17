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

}