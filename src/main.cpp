#define ASIO_STANDALONE

#include <time.h>
#include <iostream>
#include <ostream>
#include <filesystem>

#include "CLog.h"
#include "CSettings.h"
#include "Constants.h"
#include "CThreadPool.h"
#include "CDatabase.h"
#include "CClient.h"
#include "CFrontClient.h"

#include "CServer.h"
#include "CFrontServer.h"
#include "Utility.h"
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
   
    BaseLib::ThreadPool->Initialize(std::jthread::hardware_concurrency());
    BaseLib::Database->Initialize("MegaVoltsPP", "127.0.0.1", 3307, "root", "ngiga123");
   /* auto started = std::chrono::high_resolution_clock::now();
    for (int i = 0; i < 100000; i++)
    {
        auto task = BaseLib::ThreadPool->post([i]()
            {
                std::string username = "sw1ndle" + std::to_string(i);
                auto hash = Utility::Hash("god123");
                auto auth_key = Utility::GenerateAuthKey(username, "god123");
                BaseLib::Database->InsertFrontAccount(username, hash.first, hash.second, 2, auth_key);
                
            });
        task.wait();
        
    }
    auto done = std::chrono::high_resolution_clock::now();
    std::printf("took %d ms to insert 100k accounts\n", std::chrono::duration_cast<std::chrono::milliseconds>(done - started).count());*/
    /*
    BaseLib::FrontAccount frontAccount;
    std::string username = "sw1ndle";
    Utility::ToLowercase(username);
    std::string password = "god123";
    auto started = std::chrono::high_resolution_clock::now();
    dp::thread_pool pool(std::jthread::hardware_concurrency());
    for (int i = 0; i < 1000; i++)
    {
        auto task = pool.enqueue([i, username, &frontAccount]
            {
                return BaseLib::Database->GetFrontAccount(username, &frontAccount);
           
            });
        auto AccountFound = task.get();
        pool.enqueue_detach([i, frontAccount, AccountFound]
            {
                if (AccountFound)
                {
                    //std::printf("Index:%d\n", frontAccount.Index);
                    //std::printf("Username:%s\n", frontAccount.Username.c_str());
                    //std::printf("PassHash:%s\n", frontAccount.Password.c_str());
                    //std::printf("Salt:%s\n", frontAccount.Salt.c_str());
                    //std::printf("Grade:%d\n", frontAccount.Grade);
                    //std::printf("(%d) AuthKey:%llu\n", i, frontAccount.AuthKey);
                }
                //else { std::printf("Account not found\n"); }
            });
        //
    }
    auto done = std::chrono::high_resolution_clock::now();
    std::printf("took %d ms to check 1k accounts\n", std::chrono::duration_cast<std::chrono::milliseconds>(done - started).count());


    auto started2 = std::chrono::high_resolution_clock::now();
    for (int i = 0; i < 1000; i++)
    {
        auto task = BaseLib::ThreadPool->post([i, username, &frontAccount]()
            {
                auto AccountFound = BaseLib::Database->GetFrontAccount(username, &frontAccount);
        BaseLib::ThreadPool->post([i, AccountFound, frontAccount]() {
            if (AccountFound)
            {
                //std::printf("Index:%d\n", frontAccount.Index);
                //std::printf("Username:%s\n", frontAccount.Username.c_str());
                //std::printf("PassHash:%s\n", frontAccount.Password.c_str());
                //std::printf("Salt:%s\n", frontAccount.Salt.c_str());
                //std::printf("Grade:%d\n", frontAccount.Grade);
                //std::printf("(%d) AuthKey:%llu\n", i, frontAccount.AuthKey);
            }
            else { std::printf("Account not found\n"); }
            });

            });
        task.wait();
    }
    auto done2 = std::chrono::high_resolution_clock::now();
    std::printf("took %d ms to check 1k accounts\n", std::chrono::duration_cast<std::chrono::milliseconds>(done2 - started2).count());
    
    */


/*
    if (BaseLib::Database->GetFrontAccount(username, &frontAccount))
    {
        std::printf("sw1ndle:god123 works: %s\n", Utility::IsPasswordValid(password, frontAccount.Password, frontAccount.Salt) ? "true" : "false");
    }
    else
    {
        
        auto hash = Utility::Hash(password);
        auto auth_key = Utility::GenerateAuthKey(username, password);
        BaseLib::Database->InsertFrontAccount(username, hash.first, hash.second, 2, auth_key);
    }

    */
    
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