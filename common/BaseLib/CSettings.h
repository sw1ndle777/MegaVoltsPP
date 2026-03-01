#pragma once

#include <iostream>
#include <fstream>
#include <string>
#include <memory>
#include <vector>
#include <filesystem>
#include <rapidjson/document.h>
#include <rapidjson/prettywriter.h>
#include <rapidjson/stringbuffer.h>

namespace BaseLib
{
    class CSettings
    {
    public:

        struct HostSettings
        {
            std::string host;
            uint32_t port;
            uint32_t ipc_port;
            uint32_t asio_threads;
            uint32_t database_threads;
			uint32_t logger_threads;
            uint32_t playtime_min_seconds;
            bool debug;
            bool watchguard;
            bool gacha_pity_enabled;
        };
        struct DatabaseSettings
        {
            std::string host;
            uint32_t port;
            std::string db_name;
            std::string user;
            std::string password;
        };
        struct WebsiteSettings
        {
            std::string host;
            uint32_t port;
            uint32_t timeout;
        };
        struct ServerSettings
        {
            HostSettings front;
            HostSettings main;
            HostSettings cast;
            DatabaseSettings database;
            WebsiteSettings website;
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
        rapidjson::Document config_root;
    };

    extern std::unique_ptr<CSettings> DefaultSettings;
}

//#endif