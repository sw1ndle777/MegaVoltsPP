#pragma once
#include <BaseLib/CThreadPool.h>
#include <fstream>
#include <string>
#include <memory>
#include <cstdarg>
#include <filesystem>
#include <shared_mutex>
#include <source_location>
#include <ctime>
#include <chrono>
#include "fmt/format.h"
#include "fmt/color.h"
#include <regex>
#include <format>

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
            
            const auto file_name = extractFileName(source_location.file_name());
            std::string function_name = extractFunctionName(source_location.function_name());
            std::string source_debug_info = fmt::format("({}:{}) {}() ", file_name, source_location.line(), function_name);
            std::string formattedMessage = fmt::vformat(format, fmt::make_format_args(args...));
            {
                std::unique_lock<std::mutex> lock(queueMutex);
                logQueue.push({ source_debug_info + formattedMessage, { source_debug_info, formattedMessage, color } });
            }
            
			if(!isProcessing.exchange(true))
			{
				BaseLib::LogPool->submit_task([this]() mutable
				{
					std::pair<std::string,std::tuple<std::string,std::string,fmt::color>> logEntry;
					while(true)
					{
						{
							std::unique_lock<std::mutex> lock(queueMutex);
							if(logQueue.empty())
							{
								isProcessing = false;
								break;
							}
							logEntry = std::move(logQueue.front());
							logQueue.pop();
						}
						auto& [text,printData] = logEntry;
						Write(text);
						auto& [source_debug_info,formattedMessage,color] = printData;
						fmt::print(fg(fmt::color::purple) | fmt::emphasis::bold,"{}",source_debug_info.c_str());
						std::regex re(R"(([^()]*)(\([^()]*\)))");
						std::smatch match;
						std::string::const_iterator search_start(formattedMessage.cbegin());
						bool changeColorForRareParenthese = false;
						auto changecolor = fmt::color::green;
						while(std::regex_search(search_start,formattedMessage.cend(),match,re))
						{
							fmt::print(fg(color) | fmt::emphasis::bold,"{}",match[1].str().c_str());
							if(match[1].str().find("rare") != std::string::npos) changecolor = fmt::color::yellow;
							if(match[1].str().find("normal") != std::string::npos) changecolor = fmt::color::gray;
							fmt::print(fg(changecolor) | fmt::emphasis::bold,"{}",match[2].str().c_str());
							changecolor = fmt::color::green;
							search_start = match.suffix().first;
						}
						fmt::print(fg(color) | fmt::emphasis::bold,"{}\n",std::string(search_start,formattedMessage.cend()));
					}
				},BS::pr::low);
			}
            
        }
		/*
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

                    fmt::print(fg(fmt::color::purple) | fmt::emphasis::bold, "{}", source_debug_info.c_str());

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
		*/
    private:
        
        std::queue<std::pair<std::string, std::tuple<std::string, std::string, fmt::color>>> logQueue;
        std::mutex queueMutex;
        std::condition_variable cv;
        //std::optional<std::jthread> logThread;
        std::atomic<bool> stopLogging;

        std::mutex WriteMutex;
        std::ofstream File;
    };

    extern std::unique_ptr<CLog> EventLog;
}

//#endif