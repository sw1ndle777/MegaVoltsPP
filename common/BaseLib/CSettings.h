#pragma once
#ifdef _DEBUG
#pragma comment(lib, "jsoncpp_d.lib")
#else
#pragma comment(lib, "jsoncpp.lib")
#endif

#include <iostream>
#include <fstream>
#include <string>
#include <memory>
#include <vector>

#include <json.h>

namespace BaseLib
{
    class CSettings
    {
    public:

        struct HostSettings
        {
            std::string host;
            std::uint32_t port;
            std::uint32_t ipc_port;
            std::uint32_t asio_threads;
            //std::uint32_t pool_threads;
            bool debug;
            bool watchguard;
        };
        struct DatabaseSettings
        {
            std::string host;
            std::uint32_t port;
            std::string db_name;
            std::string user;
            std::string password;
        };

        struct ServerSettings
        {
            HostSettings front;
            HostSettings main;
            HostSettings cast;
            DatabaseSettings database;
        };


    public:
        CSettings() {};
        ~CSettings() {};

        bool LoadOptions();

        ServerSettings GetServerSettings();

    private:
        bool settingsLoaded = false;
        const char* fileName = "settings.json";

        std::ifstream config_doc;
        Json::Value config_root;
    };

    extern std::unique_ptr<CSettings> DefaultSettings;
}

//#endif