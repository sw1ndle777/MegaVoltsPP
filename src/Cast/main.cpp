#define ASIO_STANDALONE

#include <time.h>
#include <iostream>
#include <ostream>
#include <filesystem>

#include <BaseLib/CThreadPool.h>
#include "BaseLib/CLog.h"
#include "BaseLib/CSettings.h"
#include "NetEngine/Constants.h"
#include "BaseLib/CDatabase.h"

#include "NetEngine/CServer.h"
#include "CCastServer.h"
#include "BaseLib/Utility.h"
//#include <fmt/color.h>
//#include "BaseLib/CCrashHandler.h"
std::ostream& outputStream = std::cout;

using namespace NetEngine::Packets::Cast;



#define NOMINMAX
#include <client/crashpad_client.h>
#include <client/crash_report_database.h>
#include <client/settings.h>
#include <client/crashpad_info.h>
#include <util/file/file_io.h>
#include <util/misc/paths.h>

#pragma comment(lib,"client/client.lib")
#pragma comment(lib, "client/common.lib")
#pragma comment(lib,"util/util.lib")
#pragma comment(lib, "third_party/mini_chromium/mini_chromium/base/base.lib")

#if  defined(__linux__)
typedef std::string StringType;
#elif defined(_MSC_VER)
typedef std::wstring StringType;
#endif

StringType getExecutableDir() 
{
    HMODULE hModule = GetModuleHandleW(NULL);
    WCHAR path[MAX_PATH];
    DWORD retVal = GetModuleFileNameW(hModule, path, MAX_PATH);
    if (retVal == 0) return L"";

    wchar_t *lastBackslash = wcsrchr(path, '\\');
    if (lastBackslash == NULL) return L"";
    *lastBackslash = 0;

    return path;
}

void init_crash_handler()
{
    StringType exeDir = L"..";
    base::FilePath handler(exeDir + L"\\crash_dumps\\crashpad_handler.exe");
    base::FilePath db_path(exeDir + L"\\crash_dumps\\cast\\");
    base::FilePath metrics_path(exeDir + L"\\crash_dumps\\cast\\metrics\\");

    std::map<std::string, std::string> annotations;
    annotations["format"] = "minidump";           // Required: Crashpad setting to save crash as a minidump
    annotations["database"] = "mvo_cast_crash_db";             // Required: BugSplat appName
    annotations["product"] = "mvo_cast_gs"; // Required: BugSplat appName
    annotations["version"] = "1.0.0";             // Required: BugSplat appVersion

    std::vector<std::string> arguments;
    arguments.push_back("--no-rate-limit"); // optional

    std::unique_ptr<crashpad::CrashReportDatabase> database = crashpad::CrashReportDatabase::Initialize(db_path);
    if (database == NULL) return;

    crashpad::CrashpadClient *client = new crashpad::CrashpadClient();
    bool success = client->StartHandler(
        handler,
        db_path,
        metrics_path,
        "",              // URL for crash reports — leave empty for local only
        annotations,
        arguments,
        true,            // restartable
        false             // asynchronous start
    );
}

int main()
{
    init_crash_handler();
    //HANDLE m_process_handle = OpenProcess(PROCESS_QUERY_INFORMATION | PROCESS_VM_READ, false, GetCurrentProcessId());
    //Utility::GetCpuUsage(m_process_handle);
    //CloseHandle(m_process_handle);
    //CrashHandler::Init("../crash_dumps/MegaVoltsPP_cast.dmp");
    HANDLE hOut = GetStdHandle(STD_OUTPUT_HANDLE);
    DWORD dwMode = 0;
    GetConsoleMode(hOut, &dwMode);
    dwMode |= ENABLE_VIRTUAL_TERMINAL_PROCESSING;
    SetConsoleMode(hOut, dwMode);//enable colors

    std::srand(static_cast<uint32_t>(std::time(NULL)));
    BaseLib::DefaultSettings->LoadOptions();
    const auto& server_settings = BaseLib::DefaultSettings->GetServerSettings();
    BaseLib::LogPool = std::make_unique<BS::thread_pool<BS::tp::priority>>(server_settings.cast.logger_threads);

    BaseLib::EventLog->Initialize("../logs/MegaVoltsPP_cast.log", false);
    
   
    //BaseLib::ThreadPool->Initialize(std::jthread::hardware_concurrency());
    //BaseLib::Database->Initialize(server_settings.database.db_name.c_str(), server_settings.database.host.c_str(), server_settings.database.port, server_settings.database.user.c_str(), server_settings.database.password.c_str());

    Game::CCastServer* castServer = new Game::CCastServer();
    NetEngine::CServer::SServerSettings settings = NetEngine::CServer::SServerSettings(server_settings.cast.host.c_str(), std::to_string(server_settings.cast.port).c_str(), std::to_string(server_settings.cast.ipc_port).c_str(), server_settings.cast.debug, false, true, server_settings.cast.watchguard, server_settings.cast.asio_threads, 0, server_settings.cast.database_threads);
    castServer->Setup(settings, server_settings);
    castServer->Run();
    
    std::cin.ignore();
    return 0;
}