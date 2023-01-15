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

    CSettings::AccountSettings CSettings::GetAccountSettings()
    {
        if (!this->settingsLoaded)
        {
            this->LoadOptions();
        }

        CSettings::AccountSettings accountSettings;

        Json::Value accountSettingsValue = this->config_root["AccountSettings"];
        accountSettings.username = accountSettingsValue.get("username", "").asString();
        accountSettings.password = accountSettingsValue.get("password", "").asString();

        return accountSettings;
    }

    CSettings::ServerSettings CSettings::GetServerSettings()
    {
        if (!this->settingsLoaded)
        {
            this->LoadOptions();
        }

        CSettings::ServerSettings serverSettings;

        Json::Value serverSettingsValue = this->config_root["ServerSettings"];
        serverSettings.front.host = serverSettingsValue["Front"].get("host", "").asString();
        serverSettings.front.port = serverSettingsValue["Front"].get("port", "").asString();
        serverSettings.main.host = serverSettingsValue["Main"].get("host", "").asString();
        serverSettings.main.port = serverSettingsValue["Main"].get("port", "").asString();
        serverSettings.cast.host = serverSettingsValue["Cast"].get("host", "").asString();
        serverSettings.cast.port = serverSettingsValue["Cast"].get("port", "").asString();

        return serverSettings;
    }

    CSettings::ChannelSettings CSettings::GetChannelSettings()
    {
        if (!this->settingsLoaded)
        {
            this->LoadOptions();
        }

        CSettings::ChannelSettings channelSettings;

        Json::Value channelSettingsValue = this->config_root["ChannelSettings"]["ids"];
        for (int index = 0; index < channelSettingsValue.size(); index++) {
            channelSettings.ids.push_back(channelSettingsValue[index].asInt());
        }

        return channelSettings;
    }

    std::unique_ptr<CSettings> DefaultSettings = std::make_unique<CSettings>();
}