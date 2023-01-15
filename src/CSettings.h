#pragma once
#ifndef CSETTINGS_H
#define CSETTINGS_H

#ifdef _DEBUG
#pragma comment(lib, "json/lib/jsoncpp_d.lib")
#else
#pragma comment(lib, "json/lib/jsoncpp.lib")
#endif

#include <iostream>
#include <fstream>
#include <string>
#include <memory>
#include <vector>

#include "json/json.h"

namespace BaseLib
{
    class CSettings
    {
    public:
        struct AccountSettings
        {
            std::string username;
            std::string password;
        };

        struct HostSettings
        {
            std::string host;
            std::string port;
        };

        struct ServerSettings
        {
            HostSettings front;
            HostSettings main;
            HostSettings cast;
        };

        struct ChannelSettings
        {
            std::vector<uint32_t> ids;
        };

    public:
        CSettings() {};
        ~CSettings() {};

        bool LoadOptions();

        AccountSettings GetAccountSettings();
        ServerSettings GetServerSettings();
        ChannelSettings GetChannelSettings();

    private:
        bool settingsLoaded = false;
        const char* fileName = "settings.json";

        std::ifstream config_doc;
        Json::Value config_root;
    };

    extern std::unique_ptr<CSettings> DefaultSettings;
}

#endif