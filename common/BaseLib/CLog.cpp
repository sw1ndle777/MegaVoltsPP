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
        std::ios_base::openmode mode = std::ofstream::out;
        mode |= removeExisting ? std::ofstream::trunc : std::ofstream::app;

        File.open(Path, mode);

        if (!File.is_open())
        {
            throw std::runtime_error("Could not open file: " + Path);
        }
    }

    void CLog::Write(const std::string& Text)
    {
        auto now = std::chrono::system_clock::now();
        auto time = std::chrono::system_clock::to_time_t(now);

        char buffer[80];
        std::strftime(buffer, sizeof(buffer), "[%d-%m-%Y %H:%M:%S]", std::localtime(&time));

		const std::string Output = fmt::format("{} {}", buffer, Text);

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


    std::unique_ptr<CLog> EventLog = std::make_unique<CLog>();
}