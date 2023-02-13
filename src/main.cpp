#define ASIO_STANDALONE

#include <time.h>
#include <iostream>
#include <ostream>
#include <filesystem>

#include "CLog.h"
#include "CSettings.h"
#include "Constants.h"

#include "CClient.h"
#include "CFrontClient.h"

#include "CServer.h"
#include "CFrontServer.h"

std::ostream& outputStream = std::cout;

using namespace NetEngine::Packets::Front;

int main()
{
    std::srand(std::time(NULL));
    std::filesystem::path cwd = std::filesystem::current_path() / "MegaVoltsPP.log";

    BaseLib::EventLog->Initialize(cwd.string(), true);
    BaseLib::EventLog->Info("MegaVoltsPP initialization..");

    BaseLib::EventLog->Info("Loading settings..");
    BaseLib::DefaultSettings->LoadOptions();
    BaseLib::EventLog->Info("Settings loaded..");

    Game::CFrontServer* frontServer = new Game::CFrontServer();
    NetEngine::CServer::SServerSettings settings = NetEngine::CServer::SServerSettings("127.0.0.1", "12000", true, true, 0);
    frontServer->Setup(settings);
    frontServer->Run();
   

    /*
    Game::CFrontClient* frontClient = new Game::CFrontClient();
    NetEngine::CClient::SClientSettings settings;

    settings.ip = "127.0.0.1";
    settings.port = "12000";
    settings.useEncryption = true;

    frontClient->Setup(settings);
    frontClient->Connect();

    while (true) // Game-Mainthread
    {
        frontClient->Update();
        std::this_thread::sleep_for(std::chrono::milliseconds(1));
    }
    */

    return 0;
}