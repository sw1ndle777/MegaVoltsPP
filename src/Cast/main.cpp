#define ASIO_STANDALONE

#include <time.h>
#include <iostream>
#include <ostream>
#include <filesystem>


#include "BaseLib/CLog.h"
#include "BaseLib/CSettings.h"
#include "NetEngine/Constants.h"
#include "BaseLib/CDatabase.h"

#include "NetEngine/CServer.h"
#include "CCastServer.h"
#include "BaseLib/Utility.h"
#include <fmt/color.h>
#include "BaseLib/CCrashHandler.h"
std::ostream& outputStream = std::cout;

using namespace NetEngine::Packets::Cast;

int main()
{
    HANDLE m_process_handle = OpenProcess(PROCESS_QUERY_INFORMATION | PROCESS_VM_READ, false, GetCurrentProcessId());
    Utility::GetCpuUsage(m_process_handle);
    CloseHandle(m_process_handle);
    CrashHandler::Init("../crash_dumps/MegaVoltsPP_cast.dmp");
    HANDLE hOut = GetStdHandle(STD_OUTPUT_HANDLE);
    DWORD dwMode = 0;
    GetConsoleMode(hOut, &dwMode);
    dwMode |= ENABLE_VIRTUAL_TERMINAL_PROCESSING;
    SetConsoleMode(hOut, dwMode);//enable colors

    std::srand(static_cast<uint32_t>(std::time(NULL)));
    BaseLib::EventLog->Initialize("../logs/MegaVoltsPP_cast.log", false);
    BaseLib::DefaultSettings->LoadOptions();
    const auto& server_settings = BaseLib::DefaultSettings->GetServerSettings();
   
    //BaseLib::ThreadPool->Initialize(std::jthread::hardware_concurrency());
    //BaseLib::Database->Initialize(server_settings.database.db_name.c_str(), server_settings.database.host.c_str(), server_settings.database.port, server_settings.database.user.c_str(), server_settings.database.password.c_str());

    Game::CCastServer* castServer = new Game::CCastServer();
    NetEngine::CServer::SServerSettings settings = NetEngine::CServer::SServerSettings(server_settings.cast.host.c_str(), std::to_string(server_settings.cast.port).c_str(), std::to_string(server_settings.cast.ipc_port).c_str(), server_settings.cast.debug, false, true, server_settings.cast.watchguard, server_settings.cast.asio_threads);
    castServer->Setup(settings, server_settings);
    castServer->Run();
    
    std::cin.ignore();
    return 0;
}