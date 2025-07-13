#define ASIO_STANDALONE
#include "BaseLib/CLog.h" 
#include <time.h>
#include <iostream>
#include <ostream>
#include <filesystem>


#include "BaseLib/CSettings.h"
#include "NetEngine/Constants.h"
#include "BaseLib/CDatabase.h"

#include "NetEngine/CServer.h"



#include "CFrontServer.h"
#include "BaseLib/Utility.h"
#include <fmt/color.h>

#define NOMINMAX


#include <crashpad/client/crashpad_client.h>
#include <crashpad/client/crash_report_database.h>
#include <crashpad/client/settings.h>
#include <crashpad/client/crashpad_info.h>
//#include <crashpad/util/file/file_io.h>
//#include <crashpad/util/misc/paths.h>

//#pragma comment(lib, "third_party/mini_chromium/mini_chromium/base/base.lib")

std::ostream& outputStream = std::cout;

using namespace NetEngine::Packets::Front;


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
    base::FilePath db_path(exeDir + L"\\crash_dumps\\front\\");
    base::FilePath metrics_path(exeDir + L"\\crash_dumps\\front\\metrics\\");

    std::map<std::string, std::string> annotations;
    annotations["format"] = "minidump";           // Required: Crashpad setting to save crash as a minidump
    annotations["database"] = "mvo_front_crash_db";             // Required: BugSplat appName
    annotations["product"] = "mvo_front_gs"; // Required: BugSplat appName
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


    //CrashHandler::Init("../crash_dumps/MegaVoltsPP_front.dmp");
    HANDLE hOut = GetStdHandle(STD_OUTPUT_HANDLE);
    DWORD dwMode = 0;
    GetConsoleMode(hOut, &dwMode);
    dwMode |= ENABLE_VIRTUAL_TERMINAL_PROCESSING;
    SetConsoleMode(hOut, dwMode);//enable colors

    
    std::srand(static_cast<uint32_t>(std::time(NULL)));
    BaseLib::DefaultSettings->LoadOptions();
    const auto& server_settings = BaseLib::DefaultSettings->GetServerSettings();
    BaseLib::LogPool = std::make_unique<BS::thread_pool<BS::tp::priority>>(server_settings.front.logger_threads);
	BaseLib::DbPool = std::make_unique<BS::thread_pool<BS::tp::priority>>(server_settings.front.database_threads);
    BaseLib::EventLog->Initialize("../logs/MegaVoltsPP_front.log", false);
    BaseLib::Database->Initialize(server_settings.database.db_name, server_settings.database.host, server_settings.database.port, server_settings.database.user, server_settings.database.password);

    Game::CFrontServer* frontServer = new Game::CFrontServer();
    NetEngine::CServer::SServerSettings settings = NetEngine::CServer::SServerSettings(server_settings.main.host, std::to_string(server_settings.front.port), std::to_string(server_settings.front.ipc_port), server_settings.front.debug, true, true, server_settings.cast.watchguard, server_settings.front.asio_threads, server_settings.front.database_threads, server_settings.front.logger_threads);
    frontServer->Setup(settings, server_settings);
    frontServer->Run();

    std::cin.ignore();
    return 0;
}