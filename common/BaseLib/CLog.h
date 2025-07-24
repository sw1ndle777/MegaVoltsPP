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

namespace BaseLib
{
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
        void Debug(std::source_location source_location, fmt::color color, std::string_view format, Args&&... args)
        {
            auto file_name = extractFileName(source_location.file_name());
            auto function_name = extractFunctionName(source_location.function_name());
            auto source_debug_info = fmt::format("({}:{}) {}() ", file_name, source_location.line(), function_name);
            auto formattedMessage = fmt::vformat(format, fmt::make_format_args(args...));
            {
                std::unique_lock<std::mutex> lock(queueMutex);
                logQueue.push({ source_debug_info + formattedMessage, { source_debug_info, formattedMessage, color } });
            }
            
			if(!isProcessing.exchange(true))
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
}