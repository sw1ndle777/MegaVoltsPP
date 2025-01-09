#include "CLog.h"
#include <fstream>
#include <filesystem>
#include <mutex>
#include <chrono>
#include <format>
namespace BaseLib
{

    void CLog::Initialize(const std::string& Path, bool removeExisting)
    {

        std::filesystem::path logFilePath(Path);

        // Extract the directory part of the file path
        std::filesystem::path logDirectory = logFilePath.parent_path();


        if (!std::filesystem::exists(logDirectory))
        {
            std::error_code ec;
            if (!std::filesystem::create_directories(logDirectory, ec))
            {
                std::cerr << "Failed to create directory: " << logDirectory << ", error: " << ec.message() << std::endl;
            }
        }

        if (removeExisting)
        {
            if (std::filesystem::exists(logFilePath))
                std::filesystem::remove(logFilePath);
        }

        File.open(Path, std::ofstream::out | std::ofstream::trunc);

        if (!File.is_open())
        {
            throw std::runtime_error("Could not open file: " + Path);
        }
        logThread.emplace([this] 
        {
            ProcessQueue();
        });
    }

    void CLog::Write(const std::string& Text)
    {
        auto now = std::chrono::system_clock::now();
        auto time = std::chrono::system_clock::to_time_t(now);

        char buffer[80];
        std::strftime(buffer, sizeof(buffer), "[%d-%m-%Y %H:%M:%S]", std::localtime(&time));

        const std::string Output = std::format("{} {}", buffer, Text);

        //std::scoped_lock lock(WriteMutex);
        File << Output << std::endl;
    }

    void CLog::Add(const char* format, ...)
    {
        char buffer[8192] = { 0 };
        va_list arglist;

        va_start(arglist, format);
        vsprintf(buffer, format, arglist);
        va_end(arglist);

        Write(buffer);
    }

    void CLog::Info(const char* format, ...)
    {
        char buffer[8192] = { 0 };
        va_list arglist;

        va_start(arglist, format);
        vsprintf(buffer, format, arglist);
        va_end(arglist);

        Write(std::format("[INFO] {}", buffer));
    }

    void CLog::Warning(const char* format, ...)
    {
        char buffer[8192] = { 0 };
        va_list arglist;

        va_start(arglist, format);
        vsprintf(buffer, format, arglist);
        va_end(arglist);

        Write(std::format("[WARNING] {}", buffer));
    }

    void CLog::Error(const char* format, ...)
    {
        char buffer[8192] = { 0 };
        va_list arglist;

        va_start(arglist, format);
        vsprintf(buffer, format, arglist);
        va_end(arglist);

        Write(std::format("[ERROR] {}", buffer));
    }

    void CLog::Verbose(const char* format, ...)
    {
        char buffer[8192] = { 0 };
        va_list arglist;

        va_start(arglist, format);
        vsprintf(buffer, format, arglist);
        va_end(arglist);

        Write(std::format("[VERBOSE] {}", buffer));
    }

    std::unique_ptr<CLog> EventLog = std::make_unique<CLog>();
}