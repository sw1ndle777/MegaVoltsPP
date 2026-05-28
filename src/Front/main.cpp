#define ASIO_STANDALONE
#include "BaseLib/CLog.h" 
#include <time.h>
#include <iostream>
#include <ostream>
#include <filesystem>
#include <fstream>
#include <cstdlib>


#include "BaseLib/CSettings.h"
#include "NetEngine/Constants.h"
#include "BaseLib/CDatabase.h"
#include "BaseLib/CDatabaseFactory.h"

#include "NetEngine/CServer.h"



#include "CFrontServer.h"
#include "BaseLib/Utility.h"

#include <fmt/color.h>

#include <crashpad/client/crashpad_client.h>
#include <crashpad/client/crash_report_database.h>
#include <crashpad/client/settings.h>
#include <crashpad/client/crashpad_info.h>


#ifdef _WIN32
#include <Windows.h>
#endif

std::ostream& outputStream = std::cout;

using namespace NetEngine::Packets::Front;

namespace
{
    std::filesystem::path resolve_runtime_path(const std::filesystem::path& path)
    {
#ifdef _WIN32
        return path;
#else
        if (path.is_absolute())
            return path;

        std::error_code ec;
        const auto exe_path = std::filesystem::canonical("/proc/self/exe", ec);
        const auto exe_dir = ec ? std::filesystem::current_path(ec) : exe_path.parent_path();
        const auto resolved_path = exe_dir / path;
        const auto normalized_path = std::filesystem::weakly_canonical(resolved_path, ec);
        return ec ? resolved_path : normalized_path;
#endif
    }

    void write_startup_log(const std::string& message)
    {
        const auto startup_log_path = resolve_runtime_path("../logs/MegaVoltsPP_front.startup.log");
        std::error_code ec;
        std::filesystem::create_directories(startup_log_path.parent_path(), ec);

        std::ofstream startup_log_file(startup_log_path, std::ios::app);
        if (startup_log_file.is_open())
            startup_log_file << message << std::endl;

        std::cerr << message << std::endl;
    }
}


void init_crash_handler()
{
    const auto crash_root = resolve_runtime_path("../crash_dumps");

    std::error_code ec;
    std::filesystem::create_directories(crash_root / "front" / "metrics", ec);

#ifdef _WIN32
    base::FilePath handler((crash_root / "crashpad_handler.exe").wstring());
    base::FilePath db_path((crash_root / "front").wstring());
    base::FilePath metrics_path = db_path.Append(FILE_PATH_LITERAL("metrics"));
#else
    base::FilePath handler((crash_root / "crashpad_handler").string());
    base::FilePath db_path((crash_root / "front").string());
    base::FilePath metrics_path((crash_root / "front" / "metrics").string());
#endif

    std::map<std::string, std::string> annotations;
    annotations["format"] = "minidump";           // Required: Crashpad setting to save crash as a minidump
    annotations["database"] = "mvo_front_crash_db";             // Required: BugSplat appName
    annotations["product"] = "mvo_front_gs"; // Required: BugSplat appName
    annotations["version"] = "1.0.0";             // Required: BugSplat appVersion

    std::vector<std::string> arguments;
    arguments.push_back("--no-rate-limit"); // optional

    std::unique_ptr<crashpad::CrashReportDatabase> database = crashpad::CrashReportDatabase::Initialize(db_path);
    if (database == NULL)
    {
        write_startup_log("[front] Crashpad database initialization failed.");
        return;
    }

    crashpad::CrashpadClient client;
    const bool handler_started = client.StartHandler(handler, db_path, metrics_path, "", annotations, arguments, true, false);
    if (!handler_started)
        write_startup_log("[front] Crashpad handler start failed.");
    else
        write_startup_log("[front] Crashpad handler started.");
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
    BaseLib::Database = BaseLib::CreateDatabase(server_settings.database.driver);
    BaseLib::Database->Initialize(server_settings.database.db_name, server_settings.database.host, server_settings.database.port, server_settings.database.user, server_settings.database.password);

    auto frontServer = std::make_unique<Game::CFrontServer>();
    NetEngine::CServer::SServerSettings settings = NetEngine::CServer::SServerSettings(server_settings.front.host, std::to_string(server_settings.front.port), std::to_string(server_settings.front.ipc_port), server_settings.front.debug, true, true, server_settings.front.watchguard, server_settings.front.asio_threads, server_settings.front.database_threads, server_settings.front.logger_threads);
    frontServer->Setup(settings, server_settings);
    frontServer->Run();
    std::cin.get();
    frontServer.reset();
    BaseLib::DbPool.reset();
    BaseLib::LogPool.reset();

    return 0;
}
