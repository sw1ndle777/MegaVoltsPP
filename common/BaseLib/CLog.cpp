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

#ifndef _WIN32
        if (!logFilePath.is_absolute())
        {
            std::error_code ec;
            std::filesystem::path exeDir;

            const auto exePath = std::filesystem::canonical("/proc/self/exe", ec);
            if (!ec)
                exeDir = exePath.parent_path();
            else
                exeDir = std::filesystem::current_path(ec);

            // Resolve relative paths against the executable directory (/app/bin in Docker),
            // so "../logs/..." lands in /app/logs and is persisted by the host volume mount.
            const auto resolvedLogFilePath = exeDir / logFilePath;
            const auto normalizedLogFilePath = std::filesystem::weakly_canonical(resolvedLogFilePath, ec);
            logFilePath = ec ? resolvedLogFilePath : normalizedLogFilePath;
        }
#endif

        std::filesystem::path logDirectory = logFilePath.parent_path();

        if (!std::filesystem::exists(logDirectory))
        {
            std::error_code ec;
            if (!std::filesystem::create_directories(logDirectory, ec))
            {
                std::cerr << "Failed to create directory: "
                    << logDirectory << ", error: "
                    << ec.message() << std::endl;
            }
        }

        if (removeExisting && std::filesystem::exists(logFilePath))
            std::filesystem::remove(logFilePath);

        std::ios_base::openmode mode = std::ofstream::out;
        mode |= removeExisting ? std::ofstream::trunc : std::ofstream::app;

        File.open(logFilePath.string(), mode);

        if (!File.is_open())
            throw std::runtime_error("Could not open file: " + logFilePath.string());
    }

    void CLog::Write(const std::string& Text)
    {
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

        const auto Output = fmt::format("{:02}-{:02}-{:04} {:02}:{:02}:{:02}.{:03} {}",
            tm_buf.tm_mday, 
            tm_buf.tm_mon + 1, 
            tm_buf.tm_year + 1900, 
            tm_buf.tm_hour, 
            tm_buf.tm_min, 
            tm_buf.tm_sec, 
            static_cast<int>(ms.count()), 
            Text);

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
