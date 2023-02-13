#pragma once
#ifndef CLOG_H
#define CLOG_H

#include <fstream>
#include <string>
#include <memory>
#include <cstdarg>
#include <filesystem>
#include <shared_mutex>
#include <source_location>
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

    private:
        std::shared_timed_mutex WriteMutex;
        std::ofstream File;
    };

    extern std::unique_ptr<CLog> EventLog;
}

#endif