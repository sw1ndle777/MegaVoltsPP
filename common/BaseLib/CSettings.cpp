#include "CSettings.h"

namespace BaseLib
{
    bool CSettings::LoadOptions()
    {
        if (this->settingsLoaded)
        {
            return true;
        }

        if (!std::filesystem::exists(this->fileName))
        {
            std::cerr << "Config file not found, creating default settings.json...\n";
            std::ofstream defaultConfig(this->fileName);

            if (!defaultConfig.is_open())
            {
                std::cerr << "Failed to create default settings.json\n";
                return false;
            }

            rapidjson::Document doc;
            doc.SetObject();
            rapidjson::Document::AllocatorType& allocator = doc.GetAllocator();

            rapidjson::Value servers(rapidjson::kObjectType);

            auto createHostSettings = [&](const char* name, uint32_t port, uint32_t ipc, bool debug) {
                rapidjson::Value obj(rapidjson::kObjectType);
                obj.AddMember("host", "0.0.0.0", allocator);
                obj.AddMember("port", port, allocator);
                obj.AddMember("ipc_port", ipc, allocator);
                obj.AddMember("asio_threads", 0, allocator);
                obj.AddMember("database_threads", 0, allocator);
				obj.AddMember("logger_threads", 0, allocator);
                obj.AddMember("playtime_min_seconds", 0, allocator);
                obj.AddMember("debug", debug, allocator);
                obj.AddMember("watchguard", false, allocator);
                obj.AddMember("gacha_pity_enabled", false, allocator);
                obj.AddMember("move_batch", false, allocator);
                obj.AddMember("move_batch_hz", 128, allocator);
                rapidjson::Value hb(rapidjson::kObjectType);
                hb.AddMember("url", "", allocator); // empty = use global heartbeat (or disabled)
                hb.AddMember("interval_sec", 30, allocator);
                obj.AddMember("heartbeat", hb, allocator);
                servers.AddMember(rapidjson::Value(name, allocator), obj, allocator);
            };

            createHostSettings("front", 13000, 12000, false);
            createHostSettings("main", 13005, 12005, false);
            createHostSettings("cast", 13006, 12006, false);

            rapidjson::Value database(rapidjson::kObjectType);
            database.AddMember("driver", "mariadb", allocator);
            database.AddMember("host", "127.0.0.1", allocator);
            database.AddMember("port", 3306, allocator);
            database.AddMember("db_name", "", allocator);
            database.AddMember("user", "", allocator);
            database.AddMember("password", "", allocator);
            servers.AddMember("database", database, allocator);

            rapidjson::Value website(rapidjson::kObjectType);
            website.AddMember("host", "127.0.0.1", allocator);
            website.AddMember("port", 80, allocator);
            website.AddMember("timeout", 2000, allocator);
            servers.AddMember("website", website, allocator);

            rapidjson::Value heartbeat(rapidjson::kObjectType);
            heartbeat.AddMember("url", "", allocator); // empty = disabled
            heartbeat.AddMember("interval_sec", 30, allocator);
            servers.AddMember("heartbeat", heartbeat, allocator);

            doc.AddMember("servers", servers, allocator);

            rapidjson::StringBuffer buffer;
            rapidjson::PrettyWriter<rapidjson::StringBuffer> writer(buffer);
            doc.Accept(writer);

            defaultConfig << buffer.GetString();
            defaultConfig.close();
            std::cerr << "Default settings.json created successfully.\n";
        }

        this->config_doc.open(this->fileName);
        if (!this->config_doc.is_open())
        {
            return false;
        }

        std::string content((std::istreambuf_iterator<char>(config_doc)), std::istreambuf_iterator<char>());
        config_doc.close();

        config_root.Parse(content.c_str());
        if (config_root.HasParseError())
            return false;

        settingsLoaded = true;
        return true;
    }


    CSettings::ServerSettings CSettings::GetServerSettings()
    {
        if (!settingsLoaded)
            LoadOptions();

        ServerSettings serverSettings;
        const auto& servers = config_root["servers"];

        auto getHostSettings = [&](const char* name, HostSettings& settings) {
            const auto& obj = servers[name];
            settings.host = obj["host"].GetString();
            settings.port = obj["port"].GetUint();
            settings.ipc_port = obj["ipc_port"].GetUint();
            settings.asio_threads = obj["asio_threads"].GetUint();
            settings.database_threads = obj["database_threads"].GetUint();
			settings.logger_threads = obj["logger_threads"].GetUint();
            try {
                settings.playtime_min_seconds = obj["playtime_min_seconds"].GetUint();
            }
            catch (...) {
                settings.playtime_min_seconds = 90;
            }
            settings.debug = obj["debug"].GetBool();
            settings.watchguard = obj["watchguard"].GetBool();
            try { settings.gacha_pity_enabled = obj["gacha_pity_enabled"].GetBool(); }
            catch (...) { settings.gacha_pity_enabled = false; }
            try { settings.move_batch = obj["move_batch"].GetBool(); }
            catch (...) { settings.move_batch = false; }
            try { settings.move_batch_hz = obj["move_batch_hz"].GetUint(); }
            catch (...) { settings.move_batch_hz = 128; }

            // Optional per-server heartbeat. Empty url here falls back to the
            // global "heartbeat" block (resolved later by the server).
            settings.heartbeat.url = "";
            settings.heartbeat.interval_sec = 30;
            if (obj.HasMember("heartbeat") && obj["heartbeat"].IsObject())
            {
                const auto& hb = obj["heartbeat"];
                if (hb.HasMember("url") && hb["url"].IsString())
                    settings.heartbeat.url = hb["url"].GetString();
                if (hb.HasMember("interval_sec") && hb["interval_sec"].IsUint())
                    settings.heartbeat.interval_sec = hb["interval_sec"].GetUint();
            }
            if (settings.heartbeat.interval_sec == 0)
                settings.heartbeat.interval_sec = 30;
        };

        getHostSettings("front", serverSettings.front);
        getHostSettings("main", serverSettings.main);
        getHostSettings("cast", serverSettings.cast);

        const auto& db = servers["database"];
        serverSettings.database.driver = db.HasMember("driver") ? db["driver"].GetString() : "mariadb";
        serverSettings.database.host = db["host"].GetString();
        serverSettings.database.port = db["port"].GetUint();
        serverSettings.database.db_name = db["db_name"].GetString();
        serverSettings.database.user = db["user"].GetString();
        serverSettings.database.password = db["password"].GetString();

        const auto& web = servers["website"];
        serverSettings.website.host = web["host"].GetString();
        serverSettings.website.port = web["port"].GetUint();
        serverSettings.website.timeout = web["timeout"].GetUint();

        // Optional heartbeat section. Absent/empty url leaves the feature disabled.
        serverSettings.heartbeat.url = "";
        serverSettings.heartbeat.interval_sec = 30;
        if (servers.HasMember("heartbeat") && servers["heartbeat"].IsObject())
        {
            const auto& hb = servers["heartbeat"];
            if (hb.HasMember("url") && hb["url"].IsString())
                serverSettings.heartbeat.url = hb["url"].GetString();
            if (hb.HasMember("interval_sec") && hb["interval_sec"].IsUint())
                serverSettings.heartbeat.interval_sec = hb["interval_sec"].GetUint();
        }
        if (serverSettings.heartbeat.interval_sec == 0)
            serverSettings.heartbeat.interval_sec = 30;

        return serverSettings;
    }


    std::unique_ptr<CSettings> DefaultSettings = std::make_unique<CSettings>();
}
