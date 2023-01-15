#include "CLog.h"

namespace BaseLib
{
    void CLog::Initialize(const std::string& Path, bool removeExisting)
    {
        if (removeExisting)
        {
            std::filesystem::remove(Path);
        }

        File.open(Path, std::ofstream::out | std::ofstream::app);

        if (!File.is_open())
        {
#pragma warning(disable: 26444)
            std::runtime_error("Could not open file: " + Path);
        }
    }

    void CLog::Write(const std::string& Text)
    {
        time_t rawtime = std::time(0);
        tm time;

        localtime_s(&time, &rawtime);

        char buffer[80];

        strftime(buffer, sizeof(buffer), "[%d-%m-%Y %H:%M:%S]", &time);
        const std::string Output = std::string(buffer) + " " + Text;

        WriteMutex.lock_shared();
        File << Output << std::endl;
        WriteMutex.unlock_shared();
    }

    void CLog::Add(const char* format, ...)
    {
        char buffer[4096] = { 0 };
        va_list arglist;

        va_start(arglist, format);
        vsprintf(buffer, format, arglist);
        va_end(arglist);

        Write(buffer);
    }

    void CLog::Info(const char* format, ...)
    {
        char buffer[4096] = { 0 };
        va_list arglist;

        va_start(arglist, format);
        vsprintf(buffer, format, arglist);
        va_end(arglist);

        Write("[INFO] " + std::string(buffer));
    }

    void CLog::Warning(const char* format, ...)
    {
        char buffer[4096] = { 0 };
        va_list arglist;

        va_start(arglist, format);
        vsprintf(buffer, format, arglist);
        va_end(arglist);

        Write("[WARNING] " + std::string(buffer));
    }

    void CLog::Error(const char* format, ...)
    {
        char buffer[4096] = { 0 };
        va_list arglist;

        va_start(arglist, format);
        vsprintf(buffer, format, arglist);
        va_end(arglist);

        Write("[ERROR] " + std::string(buffer));
    }

    void CLog::Verbose(const char* format, ...)
    {
        char buffer[4096] = { 0 };
        va_list arglist;

        va_start(arglist, format);
        vsprintf(buffer, format, arglist);
        va_end(arglist);

        Write("[VERBOSE] " + std::string(buffer));
    }

    std::unique_ptr<CLog> EventLog = std::make_unique<CLog>();
}