#include "CCrashHandler.h"
#include <BaseLib/CLog.h>
std::string dumpFile;
constinit static std::atomic<bool> g_isFinal = false;
constinit static std::atomic<bool> g_isFinalStep = false;
constinit static std::atomic<bool> g_isClosing = false;
constinit static _EXCEPTION_POINTERS* g_p{};
constexpr size_t g_crashStackSize = 1024 * 1024 * 1;
constinit static char* g_crashStack{};
DWORD g_crashThread{};
DWORD g_traceThread{};

static std::string stackwalker_backtrace_json;
static std::stacktrace trace;
static std::string header_message;
static int crash_signal = 0; // 0 is not a valid signal id
static std::mutex mut;
static std::condition_variable cv;
static std::thread output_thread;
enum class program_status
{
    running = 0,
    crashed = 1,
    ending = 2,
    normal_exit = 3,
};
static std::atomic<program_status> status = program_status::running;
static void json_pretty_print(std::ostream& os, const Json::Value& jv, std::string* indent = nullptr)
{
    std::string indent_;
    if (!indent)
        indent = &indent_;

    switch (jv.type())
    {
        case Json::objectValue:
        { // Handle JSON object
            os << "{\n";
            indent->append(4, ' ');
            bool first = true;
            for (const auto& key : jv.getMemberNames())
            {
                if (!first)
                {
                    os << ",\n";
                }
                first = false;
                os << *indent << "\"" << key << "\" : ";
                json_pretty_print(os, jv[key], indent);
            }
            os << "\n";
            indent->resize(indent->size() - 4);
            os << *indent << "}";
            break;
        }

        case Json::arrayValue:
        { // Handle JSON array
            os << "[\n";
            indent->append(4, ' ');
            bool first = true;
            for (const auto& item : jv)
            {
                if (!first)
                {
                    os << ",\n";
                }
                first = false;
                os << *indent;
                json_pretty_print(os, item, indent);
            }
            os << "\n";
            indent->resize(indent->size() - 4);
            os << *indent << "]";
            break;
        }

        case Json::stringValue:
        { // Handle string values
            os << "\"" << jv.asString() << "\"";
            break;
        }

        case Json::uintValue: // Handle unsigned integers
            os << "0x" << std::hex << jv.asUInt64() << std::dec;
            break;

        case Json::intValue: // Handle signed integers
            os << "0x" << std::hex << jv.asInt64() << std::dec;
            break;

        case Json::realValue: // Handle double/float
            os << jv.asDouble();
            break;

        case Json::booleanValue: // Handle booleans
            os << (jv.asBool() ? "true" : "false");
            break;

        case Json::nullValue: // Handle null values
            os << "null";
            break;
    }

    if (indent->empty())
    {
        os << "\n"; // Final newline after complete JSON
    }
}
std::string GetCurrentTimestamp()
{
    auto now = std::chrono::system_clock::now();
    auto timeT = std::chrono::system_clock::to_time_t(now);
    std::tm localTime;

    localtime_s(&localTime, &timeT);

    // Use std::ostringstream to build the timestamp string
    std::ostringstream oss;
    oss << std::put_time(&localTime, "%Y%m%d_%H%M%S");  // Format: YYYYMMDD_HHMMSS
    return oss.str();
}
std::string GenerateCrashLogFilePath(const std::string& baseFileName)
{
    std::filesystem::path basePath(baseFileName);
    std::string timestamp = GetCurrentTimestamp();

    // Create the new file name with the timestamp and new extension
    std::string newFileName = basePath.stem().string() + "_" + timestamp + "_crash.log";

    // Combine the directory and file name
    std::filesystem::path fullPath = basePath.parent_path() / newFileName;

    // Normalize the path to use the appropriate directory separator for the platform
    fullPath = fullPath.make_preferred();  // This will convert slashes to backslashes on Windows

    return fullPath.string();
}
std::string GenerateStacktraceJsonFilePath(const std::string& baseFileName)
{
    std::filesystem::path basePath(baseFileName);
    std::string timestamp = GetCurrentTimestamp();

    // Create the new file name with the timestamp and new extension
    std::string newFileName = basePath.stem().string() + "_" + timestamp + "_crash_stacktrace.json";

    // Combine the directory and file name
    std::filesystem::path fullPath = basePath.parent_path() / newFileName;

    // Normalize the path to use the appropriate directory separator for the platform
    fullPath = fullPath.make_preferred();  // This will convert slashes to backslashes on Windows

    return fullPath.string();
}
std::string GenerateDumpFilePath(const std::string& baseFileName)
{
    std::filesystem::path basePath(baseFileName);
    std::string timestamp = GetCurrentTimestamp();

    // Create the new file name with the timestamp
    std::string newFileName = basePath.stem().string() + "_" + timestamp + basePath.extension().string();

    // Combine the directory and file name
    std::filesystem::path fullPath = basePath.parent_path() / newFileName;

    // Normalize the path to use the appropriate directory separator for the platform
    fullPath = fullPath.make_preferred();  // This will convert slashes to backslashes on Windows

    return fullPath.string();
}
void set_crashlog_header_message(std::string message) 
{
    std::unique_lock<std::mutex> lk(mut);
    if (status != program_status::running) return;
    header_message = message;
}
std::string get_crashlog_header_message() 
{
    std::unique_lock<std::mutex> lk(mut);
    return header_message;
}
static const char* try_get_signal_name(int signal) {
    switch (signal)
    {
        case SIGTERM:
            return "SIGTERM";
        case SIGSEGV:
            return "SIGSEGV";
        case SIGINT:
            return "SIGINT";
        case SIGILL:
            return "SIGILL";
        case SIGABRT:
            return "SIGABRT";
        case SIGFPE:
            return "SIGFPE";
    }
    return "";
}

static void output_crash_log() 
{
    auto path = GenerateCrashLogFilePath(dumpFile);
    auto json_path = GenerateStacktraceJsonFilePath(dumpFile);
    std::ofstream log(path);
    if (!header_message.empty()) log << header_message << std::endl;
    if (crash_signal != 0)
    {
        log << "Received signal " << crash_signal << " " << try_get_signal_name(crash_signal) << std::endl;
    }

    log << trace;
    log.close();
    std::ofstream stacktraceLog(json_path);
    stacktraceLog << stackwalker_backtrace_json;
    stacktraceLog.close();
}

static void crash_handler_thread() 
{
    //wait for the program to crash or exit normally
    std::unique_lock<std::mutex> lk(mut);
    cv.wait(lk, [] { return status != program_status::running; });
    lk.unlock();

    //if it crashed, output the crash log
    if (status == program_status::crashed)
    {
        output_crash_log();
    }

    //alert the crashing thread we're done with the crash log so it can finish crashing
    status = program_status::ending;
    cv.notify_one();
}

class XStackWalker : public StackWalker
{
public:
    XStackWalker() : StackWalker() {
    };
    Json::Value backtrace = Json::arrayValue;
    uint32_t frame{};

    std::vector<uint32_t> printed_bases;

    auto has_base(const uintptr_t base) {
        return std::ranges::find(printed_bases, base) != printed_bases.end();
    }

protected:
    virtual void OnOutput(LPCSTR) override {}

    virtual void OnCallstackEntry(CallstackEntryType eType, CallstackEntry& entry) override
    {
        if (backtrace.size() >= 10) return;

        std::string moduleName = entry.moduleName;
        uintptr_t offset = entry.offset;
        uintptr_t base = entry.baseOfImage;

        Json::Value jentry(Json::arrayValue);
        jentry.append(frame++);
        jentry.append(static_cast<Json::UInt64>(offset));
        jentry.append(moduleName);
        if (!has_base(base))
        {
            jentry.append(static_cast<Json::UInt64>(base));
            printed_bases.emplace_back(base);
        }
        backtrace.append(std::move(jentry));
    }
};



static void save_to_file(const std::string& fileName, const std::string& data)
{
    HANDLE hFile = CreateFileA(fileName.c_str(), GENERIC_WRITE, FILE_SHARE_READ, nullptr, OPEN_ALWAYS, FILE_ATTRIBUTE_NORMAL, nullptr);
    if (hFile != INVALID_HANDLE_VALUE)
    {
        DWORD bytesWritten = 0;
        WriteFile(hFile, data.data(), static_cast<DWORD>(data.size()), &bytesWritten, nullptr);
        CloseHandle(hFile);
    }
    else
    {
        DWORD errorCode = GetLastError();
        std::cerr << "Failed to open file: " << fileName << ", Error Code: " << errorCode << "\n";
    }
}


static inline void crash_handler() 
{
    //if we crashed during a crash... ignore lol
    if (status != program_status::running) return;

    XStackWalker sw;
    sw.ShowCallstack();
   
    std::stringstream ss;
    json_pretty_print(ss, sw.backtrace);
    stackwalker_backtrace_json = ss.str();

    //save the stacktrace
    trace = std::stacktrace::current();

    //resume the monitoring thread
    status = program_status::crashed;
    cv.notify_one();

    //wait for the crash log to finish writing
    std::unique_lock<std::mutex> lk(mut);
    cv.wait(lk, [] { return status != program_status::crashed; });
}

static inline void signal_handler(int signal) {
    crash_signal = signal;
    crash_handler();
    RaiseException(EXCEPTION_BREAKPOINT, 0, 1, (ULONG_PTR*)&signal);
    //std::quick_exit(1);
}
static inline void terminator() {
    crash_handler();
    std::quick_exit(1);
}
__declspec(noinline) static LONG WINAPI exception_handler(EXCEPTION_POINTERS* exceptionInfo) 
{
    crash_handler();
    std::string dumpFilePathWithTimestamp = GenerateDumpFilePath(dumpFile);

    HANDLE hFile = CreateFileA(dumpFilePathWithTimestamp.c_str(), GENERIC_WRITE | GENERIC_READ, 0, nullptr, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, nullptr);

    if (hFile != INVALID_HANDLE_VALUE)
    {
        MINIDUMP_EXCEPTION_INFORMATION dumpInfo;
        dumpInfo.ExceptionPointers = exceptionInfo;
        dumpInfo.ThreadId = GetCurrentThreadId();
        dumpInfo.ClientPointers = TRUE;

        MINIDUMP_TYPE dumpType = (MINIDUMP_TYPE)(MiniDumpNormal | MiniDumpWithDataSegs | MiniDumpWithIndirectlyReferencedMemory);
        MiniDumpWriteDump(GetCurrentProcess(), GetCurrentProcessId(), hFile, dumpType, &dumpInfo, nullptr, nullptr);
        CloseHandle(hFile);
    }
    return EXCEPTION_CONTINUE_SEARCH;
}
static void __cdecl invalid_parameter_handler(const wchar_t*, const wchar_t*, const wchar_t*, unsigned int, uintptr_t) {
    crash_handler();
    abort();
}

//callback needed during a normal exit to shut down the thread
static inline void normal_exit() {
    status = program_status::normal_exit;
    cv.notify_one();
    output_thread.join();
}




// Helper function to create a file path with a timestamp


void CrashHandler::NotifyAndTerminate(const wchar_t* message, const wchar_t* title)
{
    if (!g_isClosing.exchange(true))
        MessageBoxW(0, message, title, MB_SYSTEMMODAL | MB_ICONERROR);

    __fastfail(-1);
    TerminateProcess(GetCurrentProcess(), -1);
}



[[gnu::noinline, clang::optnone]] void CrashHandler::on_exception_final(EXCEPTION_POINTERS* exceptionInfo)
{
    const auto threadId = GetCurrentThreadId();
    const auto* p = exceptionInfo;
    const auto thread = GetCurrentThread();

    ClipCursor(NULL);
    ShowCursor(1);

    XStackWalker sw;
    sw.ShowCallstack(thread, p->ContextRecord);
    
    Json::Value report;
    std::string backtrace;
    g_traceThread = 1;
    std::thread([&]
    {
        g_traceThread = GetCurrentThreadId();
        //auto trace = std::stacktrace::current();
        Json::Value context;
        const auto* rc = p->ContextRecord;
        context["rbp"] = Json::UInt64(rc->Rbp);
        context["rax"] = Json::UInt64(rc->Rax);
        context["rcx"] = Json::UInt64(rc->Rcx);
        context["rdx"] = Json::UInt64(rc->Rdx);
        context["rbx"] = Json::UInt64(rc->Rbx);
        context["rsi"] = Json::UInt64(rc->Rsi);
        context["rdi"] = Json::UInt64(rc->Rdi);
        /*
        Json::Value stacktrace_cp23(Json::arrayValue);
        for (const auto& frame : trace)
        {
            Json::Value entry(Json::objectValue);
            entry["function"] = frame.description();
            entry["source"] = frame.source_file();
            entry["line"] = Json::UInt64(frame.source_line());
            stacktrace_cp23.append(std::move(entry));
        }
        */
        report["code"] = Json::UInt64((uintptr_t)p->ExceptionRecord->ExceptionCode);
        report["address"] = Json::UInt64((uintptr_t)p->ExceptionRecord->ExceptionAddress); 
        report["stacktrace_ida"] = std::move(sw.backtrace);
        //report["stacktrace_msvc"] = std::move(stacktrace_cp23);
        report["context"] = std::move(context);

        std::stringstream ss;
        json_pretty_print(ss, report);
        Json::StreamWriterBuilder writer;
        writer["indentation"] = "";
        backtrace = ss.str();//Json::writeString(writer, report);

        if (constinit static bool saved = false; !saved)
        {
            const auto dmpFile = GenerateDumpFilePath(dumpFile);
            save_to_file(dmpFile, backtrace);

            saved = true;
        }
        Sleep(1000);

        g_traceThread = 0;
    }).detach();

    while (g_traceThread) Sleep(1);
    NotifyAndTerminate(L"Encountered an error, exiting now.", L"[MVO] Crashed #1");
}
[[gnu::noinline, clang::optnone]] LONG __stdcall CrashHandler::on_exception(_EXCEPTION_POINTERS* _p)
{

    g_p = _p;

    if (!g_isFinal)
    {
        switch (auto const* record = g_p->ExceptionRecord; record->ExceptionCode)
        {
            case EXCEPTION_PRIV_INSTRUCTION:
            case EXCEPTION_ACCESS_VIOLATION:
            case EXCEPTION_DATATYPE_MISALIGNMENT:
            case EXCEPTION_BREAKPOINT:
            case EXCEPTION_SINGLE_STEP:
            case EXCEPTION_ARRAY_BOUNDS_EXCEEDED:
            case EXCEPTION_FLT_DENORMAL_OPERAND:
            case EXCEPTION_FLT_DIVIDE_BY_ZERO:
            case EXCEPTION_FLT_INEXACT_RESULT:
            case EXCEPTION_FLT_INVALID_OPERATION:
            case EXCEPTION_FLT_OVERFLOW:
            case EXCEPTION_FLT_STACK_CHECK:
            case EXCEPTION_FLT_UNDERFLOW:
            case EXCEPTION_INT_DIVIDE_BY_ZERO:
            case EXCEPTION_INT_OVERFLOW:
            case EXCEPTION_IN_PAGE_ERROR:
            case EXCEPTION_ILLEGAL_INSTRUCTION:
            case EXCEPTION_NONCONTINUABLE_EXCEPTION:
            case EXCEPTION_STACK_OVERFLOW:
            case EXCEPTION_INVALID_DISPOSITION:
            case EXCEPTION_GUARD_PAGE:
            case EXCEPTION_INVALID_HANDLE:
                g_isFinal = true;
                break;

            default:
                return EXCEPTION_CONTINUE_SEARCH;
        }
    }

    auto threadId = GetCurrentThreadId();

    if (g_isFinalStep.exchange(true))
    {
        if (g_traceThread == threadId)
        {
            g_traceThread = {};
        }

        if (g_crashThread == threadId)
        {
            NotifyAndTerminate(L"Encountered a fatal error, exiting now.", L"[MVO] Crashed #2");
        }

        SuspendThread(GetCurrentThread());
    }
    g_crashThread = threadId;

    // custom stack pointer to avoid issues on stack overflow crashes
    /*
    auto targetAddr = &g_crashStack[sizeof(g_crashStack)];
    __asm {
        mov rsp, targetAddr;
        sub rsp, 16;
        mov rax, on_exception_final;
        jmp rax;
    }
    */
    /*
    auto targetAddr = &g_crashStack[sizeof(g_crashStack)];
    __asm {
        mov esp, targetAddr;
        jmp on_exception_final;
    }
    */
}

[[gnu::noinline, clang::optnone]] LONG __stdcall CrashHandler::on_final_exception(_EXCEPTION_POINTERS* p) {
    g_isFinal = true;
    on_exception(p);
    NotifyAndTerminate(L"Encountered a fatal error, exiting now.", L"[MVO] Crashed #3");
    return 0;
}

void CrashHandler::InitializeExceptionHandlers() {
    g_crashStack = new char[g_crashStackSize];
    AddVectoredExceptionHandler(TRUE, (LPTOP_LEVEL_EXCEPTION_FILTER)&on_exception_final);
    SetUnhandledExceptionFilter((LPTOP_LEVEL_EXCEPTION_FILTER)&on_exception_final);
}



void CrashHandler::Init(const std::string& dumpFilePath)
{
    dumpFile = dumpFilePath;

    std::filesystem::path dumpFileParentPath = std::filesystem::path(dumpFilePath).parent_path();

    if (!std::filesystem::exists(dumpFileParentPath))
    {
        std::error_code ec;
        if (!std::filesystem::create_directories(dumpFileParentPath, ec))
        {

            std::cerr << "Failed to create directory: " << dumpFileParentPath << ", error: " << ec.message() << std::endl;
        }
    }

#ifdef _WIN64

    output_thread = std::thread(crash_handler_thread);
    AddVectoredExceptionHandler(TRUE, exception_handler);
    SetUnhandledExceptionFilter(exception_handler);
    std::signal(SIGABRT, signal_handler);
    std::signal(SIGSEGV, signal_handler);
    std::signal(SIGILL, signal_handler);
    std::signal(SIGTERM, signal_handler);
    std::signal(SIGFPE, signal_handler);
    std::signal(SIGINT, signal_handler);
    std::set_terminate(terminator);
    _set_abort_behavior(0, _WRITE_ABORT_MSG | _CALL_REPORTFAULT);
    _set_purecall_handler(terminator);
    _set_invalid_parameter_handler(&invalid_parameter_handler);
    std::atexit(normal_exit);

    //DWORD dwModeCrash = SetErrorMode(SEM_NOGPFAULTERRORBOX);
    //SetErrorMode(dwModeCrash | SEM_NOGPFAULTERRORBOX);
    //InitializeExceptionHandlers();
    //DWORD dwModeCrash = SetErrorMode(SEM_NOGPFAULTERRORBOX);
    //SetErrorMode(dwModeCrash | SEM_NOGPFAULTERRORBOX);
    //SetUnhandledExceptionFilter((LPTOP_LEVEL_EXCEPTION_FILTER)&WindowsCrashHandler);
#elif __linux__
    RegisterLinuxSignals();
#endif
}


#ifdef _WIN64
LONG WINAPI CrashHandler::WindowsCrashHandler(EXCEPTION_POINTERS* exceptionInfo)
{
    std::string dumpFilePathWithTimestamp = GenerateDumpFilePath(dumpFile);

    HANDLE hFile = CreateFileA(dumpFilePathWithTimestamp.c_str(), GENERIC_WRITE | GENERIC_READ, 0, nullptr, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, nullptr);

    if (hFile != INVALID_HANDLE_VALUE)
    {
        MINIDUMP_EXCEPTION_INFORMATION dumpInfo;
        dumpInfo.ExceptionPointers = exceptionInfo;
        dumpInfo.ThreadId = GetCurrentThreadId();
        dumpInfo.ClientPointers = TRUE;

        MINIDUMP_TYPE dumpType = (MINIDUMP_TYPE)(MiniDumpNormal | MiniDumpWithDataSegs | MiniDumpWithIndirectlyReferencedMemory);

        // Write the minidump to the file
        MiniDumpWriteDump(GetCurrentProcess(), GetCurrentProcessId(), hFile, dumpType, &dumpInfo, nullptr, nullptr);

        std::cerr << "Crash dump saved to " << dumpFilePathWithTimestamp << std::endl;
        CloseHandle(hFile);
    }
    else
    {
        std::cerr << "Failed to create dump file: " << dumpFilePathWithTimestamp << std::endl;
    }

    return EXCEPTION_EXECUTE_HANDLER;
}
#endif

#ifdef __linux__
// Helper function to register multiple signals
void CrashHandler::RegisterLinuxSignals()
{
    signal(SIGSEGV, LinuxCrashHandler); // Handle segmentation fault
    signal(SIGABRT, LinuxCrashHandler); // Handle abort signal
    signal(SIGTERM, LinuxCrashHandler); // Handle termination signal
    signal(SIGINT, LinuxCrashHandler);  // Handle interrupt signal
    signal(SIGILL, LinuxCrashHandler);  // Handle illegal instruction signal
    signal(SIGFPE, LinuxCrashHandler);  // Handle floating-point exception
}

// Handles multiple signals on Linux and logs stack trace
void CrashHandler::LinuxCrashHandler(int sig)
{
    std::string dumpFilePathWithTimestamp = GenerateDumpFilePath(dumpFile);

    try
    {
        std::ofstream dumpFileStream(dumpFilePathWithTimestamp, std::ios::out | std::ios::binary);

        if (dumpFileStream.is_open())
        {
            // Write signal and timestamp
            dumpFileStream << "Signal: " << strsignal(sig) << "\n";
            dumpFileStream << "Timestamp: " << GetCurrentTimestamp() << "\n";

            // Capture and write stacktrace using C++23 std::stacktrace
            auto stacktrace = std::stacktrace::current();
            dumpFileStream << "Stacktrace:\n";
            for (const auto& entry : stacktrace)
            {
                dumpFileStream << entry << "\n";
            }

            dumpFileStream.close();
            std::cerr << "Crash dump saved to " << dumpFilePathWithTimestamp << std::endl;
        }
        else
        {
            std::cerr << "Failed to create dump file: " << dumpFilePathWithTimestamp << std::endl;
        }
    }
    catch (const std::exception& e)
    {
        std::cerr << "Exception in crash handler: " << e.what() << std::endl;
    }

    _exit(1);  // Exit immediately after handling the signal
}
#endif
