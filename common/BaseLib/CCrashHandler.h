#pragma once

#include <iostream>
#include <string>
#include <fstream>
#include <csignal>
#include <cstring>
#include <stacktrace> // C++23 stacktrace header
#include <filesystem>
#include <chrono>
#include <format>
#ifdef _WIN64
#include <Windows.h>
#include <DbgHelp.h>
#pragma comment(lib, "dbghelp.lib")
#elif __linux__
#include <unistd.h>
#endif

#include <StackWalker.h>
#include <ranges>
#include <thread>

class CrashHandler
{
public:
    // Initializes the crash handler and sets the output path for dump files
    static void Init(const std::string& dumpFilePath);
    [[gnu::noinline, clang::optnone]] static void on_exception_final(EXCEPTION_POINTERS* exceptionInfo);
    [[gnu::noinline, clang::optnone]] static LONG __stdcall on_exception(_EXCEPTION_POINTERS* _p);
    [[gnu::noinline, clang::optnone]] static LONG __stdcall on_final_exception(_EXCEPTION_POINTERS* p);
    static void InitializeExceptionHandlers();
    static void NotifyAndTerminate(const wchar_t* message, const wchar_t* title);

private:
    //static std::string dumpFile;

#ifdef _WIN64
    // Handles unhandled exceptions on Windows
    static LONG WINAPI WindowsCrashHandler(EXCEPTION_POINTERS* exceptionInfo);
#endif

};
