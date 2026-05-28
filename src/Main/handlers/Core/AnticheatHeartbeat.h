#pragma once
#include "secure_channel.hpp"
#include <BaseLib/CLogging.h>

namespace Game::Handlers
{
    using namespace BaseLib;
    using namespace NetEngine;
    using namespace NetEngine::Packets::Main;

    inline void AcHeartbeat(SCallbackData& callback, CMainServer* main_server)
    {
        auto session = callback.session;
        auto message = callback.message;
        if (!session || !message) return;

        auto sid = session->GetSessionId();
        std::vector<Game::Anticheat::DetectionEvent> detectionEvents;

        if (!Game::Anticheat::g_heartbeatManager.onResponse(
                sid, message->GetData(), message->GetDataSize(),
                main_server->GetIoContext(), main_server, &detectionEvents))
        {
            DEBUGLOG(dark_cyan, "sid=({}) invalid heartbeat response, disconnecting", sid);
            main_server->DisconnectPlayer(sid, Disconnect::Reason::DataError);
            return;
        }

        if (!detectionEvents.empty())
        {
            auto acc_cache = CAccount.get<shared_t>(sid);
            auto aid = acc_cache->acc_info.Index;
            auto server_id = acc_cache->server_id;
            auto hwid = acc_cache->hwid;
            acc_cache.unlock();

            auto ip = session->GetIpAddress();

            auto extractDetail = [](const Game::Anticheat::DetectionEvent& evt)
            {
                std::size_t len = 0;
                while (len < Game::Anticheat::kDetectionDetailSize && evt.detail[len] != '\0')
                    ++len;

                return std::string(evt.detail, len);
            };

            std::vector<AcDetectionLogEntry> logs;
            logs.reserve(detectionEvents.size() * 2);
            for (const auto& evt : detectionEvents)
            {
                const auto flags = AcDetection::ExpandRawFlags(evt.flag);
                const auto detail = extractDetail(evt);
                for (const auto flag : flags)
                {
                    AcDetectionLogEntry entry;
                    entry.aid = aid;
                    entry.ip = ip;
                    entry.hwid = hwid;
                    entry.detection_flag = flag;
                    entry.extra = evt.extra;
                    entry.details = detail;
                    entry.server_id = server_id;
                    logs.push_back(std::move(entry));
                }
            }

            if (!logs.empty())
            {
                [[maybe_unused]] auto ignored = BaseLib::DbPool->submit_task([logs = std::move(logs)]() mutable
                    {
                        BaseLib::Database->PersistAcDetectionLogs(logs);
                    });
            }
        }
    }
}
