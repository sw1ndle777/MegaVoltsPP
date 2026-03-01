#define ASIO_STANDALONE
#include "BaseLib/CLog.h" 
#include <time.h>
#include <iostream>
#include <ostream>
#include <filesystem>

#ifdef _WIN32
#include <Windows.h>
#include <cwchar>
#endif


#include "BaseLib/CSettings.h"
#include "NetEngine/Constants.h"
#include "BaseLib/CDatabase.h"

#include "NetEngine/CServer.h"



#include "CFrontServer.h"
#include "BaseLib/Utility.h"

#include <fmt/color.h>

#include <crashpad/client/crashpad_client.h>
#include <crashpad/client/crash_report_database.h>
#include <crashpad/client/settings.h>
#include <crashpad/client/crashpad_info.h>

std::ostream& outputStream = std::cout;

using namespace NetEngine::Packets::Front;


#ifdef _WIN32
using StringType = std::wstring;
#else
using StringType = std::string;
#endif

StringType getExecutableDir() 
{
#ifdef _WIN32
    HMODULE hModule = GetModuleHandleW(NULL);
    WCHAR path[MAX_PATH];
    DWORD retVal = GetModuleFileNameW(hModule, path, MAX_PATH);
    if (retVal == 0) return L"";

    wchar_t* lastBackslash = wcsrchr(path, L'\\');
    if (lastBackslash == NULL) return L"";
    *lastBackslash = 0;

    return path;
#else
    std::error_code ec;
    auto cwd = std::filesystem::current_path(ec);
    if (ec) return "..";
    return cwd.string();
#endif
}

void init_crash_handler()
{
#ifdef _WIN32
    StringType exeDir = L"..";
    base::FilePath handler(exeDir + L"\\crash_dumps\\crashpad_handler.exe");
    base::FilePath db_path(exeDir + L"\\crash_dumps\\front\\");
    base::FilePath metrics_path(exeDir + L"\\crash_dumps\\front\\metrics\\");
#else
    StringType exeDir = "..";
    base::FilePath handler(exeDir + "/crash_dumps/crashpad_handler");
    base::FilePath db_path(exeDir + "/crash_dumps/front/");
    base::FilePath metrics_path(exeDir + "/crash_dumps/front/metrics/");
#endif

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
	client->StartHandler(handler, db_path, metrics_path, "", annotations, arguments, true, false);
}

int main()
{
    init_crash_handler();


    //CrashHandler::Init("../crash_dumps/MegaVoltsPP_front.dmp");
#ifdef _WIN32
    HANDLE hOut = GetStdHandle(STD_OUTPUT_HANDLE);
    DWORD dwMode = 0;
    GetConsoleMode(hOut, &dwMode);
    dwMode |= ENABLE_VIRTUAL_TERMINAL_PROCESSING;
    SetConsoleMode(hOut, dwMode);//enable colors
#endif

    
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
    std::cin.get();
    delete frontServer;
    BaseLib::DbPool.reset();
    BaseLib::LogPool.reset();

    return 0;
}
