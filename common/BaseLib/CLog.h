#pragma once
#include <fstream>
#include <string>
#include <memory>
#include <cstdarg>
#include <filesystem>
#include <shared_mutex>
#include <source_location>
#include <ctime>
#include <chrono>
#include <fmt/format.h>
#include <fmt/color.h>
#include <regex>
#include <source_location>
#include <format>
#include "CThreadPool.h"
namespace BaseLib
{
    class CLog
    {
    public:
        void Initialize(const std::string& Path, bool removeExisting = false);
        void Write(const std::string& Text);
        void Add(const char* format, ...);

        void Info(const char* format, ...);
        void Warning(const char* format, ...);
        void Error(const char* format, ...);
        void Verbose(const char* format, ...);

        std::string extractFunctionName(const std::string& str)
        {
            std::string_view result = str;

            result = result.substr(0, result.find("::<"));
            result = result.substr(0, result.find('('));
            result = result.substr(result.find_last_of(' ') + 1);

            return std::string(result);
        }
        std::string extractFileName(const std::string& path) 
        {
            const auto& pos = path.find_last_of("/\\");
            return (pos != std::string::npos) ? path.substr(pos + 1) : path;
        }
        void Stop()
        {
            stopLogging = true;
            cv.notify_all();
            if (logThread && logThread->joinable())
            {
                logThread->join();
            }
        }
        ~CLog()
        {
            Stop();
            if (File.is_open())  File.close();     
        }
        template <typename... Args>
        void Debug(std::source_location source_location, fmt::color color, std::string_view format, Args&&... args)
        {
            const auto file_name = extractFileName(source_location.file_name());//std::filesystem::path(source_location.file_name()).filename().string();
            std::string function_name = extractFunctionName(source_location.function_name());

            std::string source_debug_info = std::format("({}:{}) {}() ", file_name, source_location.line(), function_name);
            std::string formattedMessage = std::vformat(format, std::make_format_args(args...));

            {
                std::unique_lock<std::mutex> lock(queueMutex);
                logQueue.push({ source_debug_info + formattedMessage, { source_debug_info, formattedMessage, color } });
            }
            cv.notify_one();

            /*
            
            Write(source_debug_info + formattedMessage);

            fmt::print(fg(fmt::color::purple) | fmt::emphasis::bold, source_debug_info.c_str());

            std::regex re(R"(([^()]*)(\([^()]*\)))");
            std::smatch match;
            std::string::const_iterator search_start(formattedMessage.cbegin());
            bool changeColorForRareParenthese = false;
            auto changecolor = fmt::color::green;
            while (std::regex_search(search_start, formattedMessage.cend(), match, re))
            {
                fmt::print(fg(color) | fmt::emphasis::bold, "{}", match[1].str().c_str());
                if (match[1].str().find("rare") != std::string::npos) changecolor = fmt::color::yellow;
                if (match[1].str().find("normal") != std::string::npos) changecolor = fmt::color::gray;

                fmt::print(fg(changecolor) | fmt::emphasis::bold, "{}", match[2].str().c_str());
                changecolor = fmt::color::green;

                search_start = match.suffix().first;
            }

            fmt::print(fg(color) | fmt::emphasis::bold, "{}\n", std::string(search_start, formattedMessage.cend()));

            */
        }

        void ProcessQueue() 
        {
            while (!stopLogging)
            {
                std::unique_lock<std::mutex> lock(queueMutex);
                cv.wait(lock, [this] { return !logQueue.empty() || stopLogging; });

                while (!logQueue.empty())
                {
                    auto [logEntry, printData] = logQueue.front();
                    logQueue.pop();
                    lock.unlock();

                    Write(logEntry);

                    auto& [source_debug_info, formattedMessage, color] = printData;

                    fmt::print(fg(fmt::color::purple) | fmt::emphasis::bold, source_debug_info.c_str());

                    std::regex re(R"(([^()]*)(\([^()]*\)))");
                    std::smatch match;
                    std::string::const_iterator search_start(formattedMessage.cbegin());
                    bool changeColorForRareParenthese = false;
                    auto changecolor = fmt::color::green;
                    while (std::regex_search(search_start, formattedMessage.cend(), match, re))
                    {
                        fmt::print(fg(color) | fmt::emphasis::bold, "{}", match[1].str().c_str());
                        if (match[1].str().find("rare") != std::string::npos) changecolor = fmt::color::yellow;
                        if (match[1].str().find("normal") != std::string::npos) changecolor = fmt::color::gray;

                        fmt::print(fg(changecolor) | fmt::emphasis::bold, "{}", match[2].str().c_str());
                        changecolor = fmt::color::green;

                        search_start = match.suffix().first;
                    }

                    fmt::print(fg(color) | fmt::emphasis::bold, "{}\n", std::string(search_start, formattedMessage.cend()));

                    lock.lock();
                }
            }
        }
    private:
        
        std::queue<std::pair<std::string, std::tuple<std::string, std::string, fmt::color>>> logQueue;
        std::mutex queueMutex;
        std::condition_variable cv;
        std::optional<std::jthread> logThread;
        std::atomic<bool> stopLogging;

        std::mutex WriteMutex;
        std::ofstream File;
    };

    extern std::unique_ptr<CLog> EventLog;
}

//#endif