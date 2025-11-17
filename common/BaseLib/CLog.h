#pragma once
#include <fmt/format.h>
#include <fmt/color.h>
#include <BaseLib/CThreadPool.h>
#include <regex>
#include <format>
#include <fstream>
#include <string>
#include <memory>
#include <cstdarg>
#include <filesystem>
#include <shared_mutex>
#include <source_location>
#include <ctime>
#include <chrono>
#include <iostream>
#include <thread>
#include <vector>
#include <future>
#include <functional>
#include <queue>
#include <mutex>
#include <condition_variable>
#include "Enums/Protocol/Orders.h"

#define MAGIC_ENUM_RANGE_MIN 0
#define MAGIC_ENUM_RANGE_MAX 600

#include <magic_enum.hpp>
#include <time.h>
namespace BaseLib
{
    enum class PacketDir { DEBUG, REQ, ACK };
    class CLog
    {
    public:
        void Initialize(const std::string& Path, bool removeExisting = false);
        void Write(const std::string& Text);
        void Add(const char* format, ...);

        void Stop()
        {
            stopLogging = true;
            cv.notify_all();
        }
        ~CLog()
        {
            Stop();
            if (File.is_open())  File.close();
        }

        std::atomic<bool> isProcessing = false;
        template <typename... Args>
        [[deprecated("Use DEBUG/PACKETLOG instead.")]]
        void DebugOld(std::source_location source_location, fmt::color color, std::string_view format, Args&&... args)
        {
            auto file_name = extractFileName(source_location.file_name());
            auto function_name = extractFunctionName(source_location.function_name());
            auto source_debug_info = fmt::format("({}:{}) {}() ", file_name, source_location.line(), function_name);
            auto formattedMessage = fmt::vformat(format, fmt::make_format_args(args...));
            {
                std::unique_lock<std::mutex> lock(queueMutex);
                logQueue.push({ source_debug_info + formattedMessage, { source_debug_info, formattedMessage, color } });
            }

            if (!isProcessing.exchange(true))
            {
                [[maybe_unused]] auto ignored_result = BaseLib::LogPool->submit_task([this]() mutable
                    {
                        std::pair<std::string, std::tuple<std::string, std::string, fmt::color>> logEntry;
                        while (true)
                        {
                            {
                                std::unique_lock<std::mutex> lock(queueMutex);
                                if (logQueue.empty()) [[unlikely]]
                                {
                                    isProcessing = false;
                                    break;
                                }
                                logEntry = std::move(logQueue.front());
                                logQueue.pop();
                            }
                            auto& [text, printData] = logEntry;
                            Write(text);
                            auto& [source_debug_info, formattedMessage, color] = printData;
                            fmt::print(fg(fmt::color::purple) | fmt::emphasis::bold, "{}", source_debug_info.c_str());

                            std::regex re(R"(([^()]*)(\([^()]*\)))");
                            std::smatch match;
                            std::string::const_iterator search_start(formattedMessage.cbegin());
                            auto changecolor = fmt::color::green;
                            while (std::regex_search(search_start, formattedMessage.cend(), match, re))
                            {
                                fmt::print(fg(color) | fmt::emphasis::bold, "{}", match[1].str().c_str());
                                const auto& token = match[1].str();
                                if (token.contains("rare")) changecolor = fmt::color::yellow;
                                if (token.contains("normal")) changecolor = fmt::color::gray;
                                fmt::print(fg(changecolor) | fmt::emphasis::bold, "{}", match[2].str().c_str());
                                changecolor = fmt::color::green;
                                search_start = match.suffix().first;
                            }
                            fmt::print(fg(color) | fmt::emphasis::bold, "{}\n", std::string(search_start, formattedMessage.cend()).c_str());
                        }
                    }, BS::pr::low);
            }
        }

        inline fmt::color dir_color(PacketDir d) {
            switch (d) {
            case PacketDir::REQ: return fmt::color::cyan;
            case PacketDir::ACK: return fmt::color::magenta;
            }
            return fmt::color::white;
        }

        template <typename... Args>
        void Debug(std::source_location source_location,
            PacketDir dir,
            EOrder order,
            fmt::color color,
            std::string_view format,
            Args&&... args
        )
        {
            const auto file_name = extractFileName(source_location.file_name());
            const auto source_debug_info = fmt::format("{}:{} ", file_name, source_location.line());

            std::string dir_part;
            if (dir != PacketDir::DEBUG) {
                auto sv = magic_enum::enum_name(dir);
                dir_part.assign(sv.data(), sv.size());
            }

            std::string order_part;
            {
                auto sv = magic_enum::enum_name(order);
                if (!sv.empty()) {
                    order_part.assign(sv.data(), sv.size());
                }
                else {
                    const auto oid = static_cast<uint16_t>(magic_enum::enum_integer(order));
                    order_part = fmt::format("{}", oid);
                }
            }

            std::string formattedMessage = fmt::vformat(format, fmt::make_format_args(args...));
            if (!dir_part.empty() && !order_part.empty())
                formattedMessage = fmt::format("{} {} | {}", dir_part, order_part, formattedMessage);

            {
                std::unique_lock<std::mutex> lock(queueMutex);
                logQueue.push({ source_debug_info + formattedMessage, { source_debug_info, formattedMessage, color } });
            }

            if (!isProcessing.exchange(true))
            {
                [[maybe_unused]] auto ignored_result = BaseLib::LogPool->submit_task([this]() mutable
                    {
                        std::pair<std::string, std::tuple<std::string, std::string, fmt::color>> logEntry;
                        while (true)
                        {
                            {
                                std::unique_lock<std::mutex> lock(queueMutex);
                                if (logQueue.empty()) [[unlikely]] { isProcessing = false; break; }
                                logEntry = std::move(logQueue.front());
                                logQueue.pop();
                            }
                            auto& [text, printData] = logEntry;
                            Write(text);

                            auto& [source_debug_info, formattedMessage, color] = printData;

                            const auto now = std::chrono::system_clock::now();
                            auto tt = std::chrono::system_clock::to_time_t(now);

                            std::tm tm_buf;
#if defined(_WIN64)
                            localtime_s(&tm_buf, &tt);
#else
                            localtime_r(&tt, &tm_buf);
#endif

                            const auto ms = std::chrono::duration_cast<std::chrono::milliseconds>(
                                now.time_since_epoch()) % std::chrono::milliseconds(1000);

                            const auto time_str = fmt::format("{:02}-{:02}-{:04} {:02}:{:02}:{:02}.{:03}",
                                tm_buf.tm_mday, tm_buf.tm_mon + 1, tm_buf.tm_year + 1900, tm_buf.tm_hour, tm_buf.tm_min, tm_buf.tm_sec, static_cast<int>(ms.count()));

                            fmt::print(fmt::fg(fmt::color::green) | fmt::emphasis::bold, "{} ", time_str);
                            fmt::print(fg(fmt::color::purple) | fmt::emphasis::bold, "{}", source_debug_info.c_str());

                            std::string_view msg_view = formattedMessage;
                            fmt::color dirCol = fmt::color::white;
                            PacketDir foundDir = PacketDir::DEBUG;
                            size_t dirPos = msg_view.find("REQ ");
                            if (dirPos == std::string_view::npos)
                                dirPos = msg_view.find("ACK ");

                            if (dirPos != std::string_view::npos) {
                                foundDir = (msg_view.substr(dirPos, 3) == "REQ") ? PacketDir::REQ : PacketDir::ACK;
                                dirCol = dir_color(foundDir);
                                auto orderStart = dirPos + 4;
                                auto sep = msg_view.find('|', orderStart);

                                if (sep != std::string_view::npos) {
                                    auto dirToken = msg_view.substr(dirPos, 3);
                                    auto orderToken = msg_view.substr(orderStart, sep - orderStart);
                                    auto rest = msg_view.substr(sep + 1);
                                    if (!rest.empty()) {
                                        auto first_non_space = std::ranges::find_if_not(rest, [](unsigned char ch) {
                                            return std::isspace(ch);
                                            });
                                        rest.remove_prefix(std::distance(rest.begin(), first_non_space));
                                    }


                                    fmt::print(fg(dirCol) | fmt::emphasis::bold, "{} ", dirToken);
                                    fmt::print(fg(dirCol) | fmt::emphasis::bold, "{} ",
                                        (foundDir == PacketDir::REQ ? "<-" : "->"));
                                    fmt::print(fg(dirCol) | fmt::emphasis::bold, "{}", orderToken);
                                    msg_view = rest;
                                }
                            }

                            static const std::regex re(
                                R"(([^()\[\]{}]*)(\((?:[^()]*)\)|\[(?:[^\[\]]*)\]|\{(?:[^{}]*)\}))",
                                std::regex::optimize);

                            std::cmatch match;
                            const char* begin = msg_view.data();
                            const char* end = begin + msg_view.size();

                            auto changecolor = fmt::color::green;

                            while (std::regex_search(begin, end, match, re)) {
                                std::string_view outside(match[1].first, match[1].length());
                                fmt::print(fmt::fg(color) | fmt::emphasis::bold, "{}", outside);

                                if (outside.find("rare") != std::string_view::npos) changecolor = fmt::color::yellow;
                                if (outside.find("normal") != std::string_view::npos) changecolor = fmt::color::gray;
                                std::string_view group(match[2].first, match[2].length());
                                fmt::print(fmt::fg(changecolor) | fmt::emphasis::bold, "{}", group);

                                changecolor = fmt::color::green;
                                begin = match.suffix().first;
                            }

                            if (begin < end)
                                fmt::print(fmt::fg(color) | fmt::emphasis::bold, "{}\n", std::string_view(begin, end - begin));
                            else
                                fmt::print("\n");

                        }
                    }, BS::pr::low);
            }
        }
    private:   
        static constexpr std::string_view extractFunctionName(std::string_view str) noexcept 
        {
            auto res = str.substr(0, str.find("::<"));
            res = res.substr(0, res.find('('));
            return res.substr(res.find_last_of(' ') + 1);
        }
        static constexpr std::string_view extractFileName(std::string_view path) noexcept 
        {
            const auto pos = path.find_last_of("/\\");
            return (pos == std::string_view::npos) ? path : path.substr(pos + 1);
        }

        std::queue<std::pair<std::string, std::tuple<std::string, std::string, fmt::color>>> logQueue;
        std::mutex queueMutex;
        std::condition_variable cv;
        std::atomic<bool> stopLogging;
        std::mutex WriteMutex;
        std::ofstream File;
    };

    extern std::unique_ptr<CLog> EventLog;

    using enum PacketDir;
    using enum fmt::color;
    using enum EOrder;

#define PACKETLOG(DIR, ORDER_ENUM, FMT, ...) \
    BaseLib::EventLog->Debug(std::source_location::current(), (DIR), (ORDER_ENUM), dark_cyan, (FMT), ##__VA_ARGS__)

#define DEBUGLOG(COLOR, FMT, ...) \
    BaseLib::EventLog->Debug(std::source_location::current(), DEBUG, NONE, (COLOR), (FMT), ##__VA_ARGS__)

}