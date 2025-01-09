#define ASIO_STANDALONE

#include <time.h>
#include <iostream>
#include <ostream>
#include <filesystem>

#include "BaseLib/CLog.h"
#include "BaseLib/CSettings.h"
#include "NetEngine/Constants.h"
#include "BaseLib/CThreadPool.h"
#include "BaseLib/CDatabase.h"

#include "NetEngine/CServer.h"
#include "CFrontServer.h"
#include "BaseLib/Utility.h"
#include <fmt/color.h>
#include "BaseLib/CCrashHandler.h"
std::ostream& outputStream = std::cout;

using namespace NetEngine::Packets::Front;

int main()
{

    CrashHandler::Init("../crash_dumps/MegaVoltsPP_front.dmp");
    HANDLE hOut = GetStdHandle(STD_OUTPUT_HANDLE);
    DWORD dwMode = 0;
    GetConsoleMode(hOut, &dwMode);
    dwMode |= ENABLE_VIRTUAL_TERMINAL_PROCESSING;
    SetConsoleMode(hOut, dwMode);//enable colors
    
    std::srand(static_cast<std::uint32_t>(std::time(NULL)));

    BaseLib::EventLog->Initialize("../logs/MegaVoltsPP_front.log", false);
    BaseLib::DefaultSettings->LoadOptions();
    const auto& server_settings = BaseLib::DefaultSettings->GetServerSettings();
    //BaseLib::ThreadPool->Initialize(server_settings.front.pool_threads);
    BaseLib::Database->Initialize(server_settings.database.db_name, server_settings.database.host, server_settings.database.port, server_settings.database.user, server_settings.database.password);

    Game::CFrontServer* frontServer = new Game::CFrontServer();
    NetEngine::CServer::SServerSettings settings = NetEngine::CServer::SServerSettings(server_settings.main.host, std::to_string(server_settings.front.port), std::to_string(server_settings.front.ipc_port), server_settings.front.debug, true, true, server_settings.cast.watchguard, server_settings.front.asio_threads);
    frontServer->Setup(settings, server_settings);
    frontServer->Run();

    std::cin.ignore();
    return 0;
}