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
        struct WebsiteSettings
        {
            std::string host;
            std::uint32_t port;
            std::uint32_t timeout;
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