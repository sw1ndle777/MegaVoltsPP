#include "CSettings.h"

namespace BaseLib
{
    bool CSettings::LoadOptions()
    {
        if (this->settingsLoaded)
        {
            return true;
        }

        this->config_doc.open(this->fileName);
        if (!this->config_doc.is_open())
        {
            return false;
        }

        Json::CharReaderBuilder builder;

        JSONCPP_STRING errs;
        if (!parseFromStream(builder, this->config_doc, &this->config_root, &errs)) {
            return false;
        }

        settingsLoaded = true;
        return true;
    }


    CSettings::ServerSettings CSettings::GetServerSettings()
    {
        if (!this->settingsLoaded)
        {
            this->LoadOptions();
        }

        CSettings::ServerSettings serverSettings;

        Json::Value serverSettingsValue = this->config_root["servers"];
        serverSettings.front.host = serverSettingsValue["front"].get("host", "").asString();
        serverSettings.front.port = serverSettingsValue["front"].get("port", 13000).asUInt();
        serverSettings.front.ipc_port = serverSettingsValue["front"].get("ipc_port", 12000).asUInt();
        serverSettings.front.asio_threads = serverSettingsValue["front"].get("asio_threads", 0).asUInt();
        serverSettings.front.debug = serverSettingsValue["front"].get("debug", false).asBool();
        serverSettings.front.watchguard = serverSettingsValue["front"].get("watchguard", false).asBool();

        serverSettings.main.host = serverSettingsValue["main"].get("host", "").asString();
        serverSettings.main.port = serverSettingsValue["main"].get("port", 13005).asUInt();
        serverSettings.main.ipc_port = serverSettingsValue["main"].get("ipc_port", 12005).asUInt();
        serverSettings.main.asio_threads = serverSettingsValue["main"].get("asio_threads", 0).asUInt();
        //serverSettings.main.pool_threads = serverSettingsValue["main"].get("pool_threads", 0).asUInt();
        serverSettings.main.debug = serverSettingsValue["main"].get("debug", false).asBool();
        serverSettings.main.watchguard = serverSettingsValue["main"].get("watchguard", false).asBool();

        serverSettings.cast.host = serverSettingsValue["cast"].get("host", "").asString();
        serverSettings.cast.port = serverSettingsValue["cast"].get("port", 13006).asUInt();
        serverSettings.cast.ipc_port = serverSettingsValue["cast"].get("ipc_port", 12006).asUInt();
        serverSettings.cast.asio_threads = serverSettingsValue["cast"].get("asio_threads", 0).asUInt();
        //serverSettings.cast.pool_threads = serverSettingsValue["cast"].get("pool_threads", 0).asUInt();
        serverSettings.cast.debug = serverSettingsValue["cast"].get("debug", false).asBool();
        serverSettings.cast.watchguard = serverSettingsValue["cast"].get("watchguard", false).asBool();

        serverSettings.database.host = serverSettingsValue["database"].get("host", "").asString();
        serverSettings.database.port = serverSettingsValue["database"].get("port", 3306).asUInt();
        serverSettings.database.db_name = serverSettingsValue["database"].get("db_name", "").asString();
        serverSettings.database.user = serverSettingsValue["database"].get("user", "").asString();
        serverSettings.database.password = serverSettingsValue["database"].get("password", "").asString();


        return serverSettings;
    }


    std::unique_ptr<CSettings> DefaultSettings = std::make_unique<CSettings>();
}